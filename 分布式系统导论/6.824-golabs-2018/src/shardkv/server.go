package shardkv

import "bytes"
import "fmt"
import "labgob"
import "labrpc"
import "log"
import "raft"
import "raftservice"
import "shardmaster"
import "sort"
import "sync"
import "time"
import "util"

// import "runtime"
// import "runtime/pprof"
// import "os"

// enable debug output
const DEBUG = false

const POLL_PERIOD  = 100 * time.Millisecond
const SLEEP_PERIOD = 10 * time.Millisecond

const UNASSIGNED = 0

func id(components ...interface{}) string {
    result := ""

    for ii, c := range components {
        if ii > 0 { result += "-" }
        result += fmt.Sprintf("%v", c)
    }

    return result
}

type ShardKV struct {
	mu           sync.Mutex
	me           int
	rf           *raft.Raft
	applyCh      chan raft.ApplyMsg
	make_end     func(string) *labrpc.ClientEnd
	gid          int
	masters      []*labrpc.ClientEnd
	maxraftstate int // snapshot if log grows this big

	persister *raft.Persister

	// service taking care of interaction with Raft
	rs *raftservice.RaftService

	// clerk for interaction with the shard master cluster
	mck *shardmaster.Clerk

	active bool
	lastIncludedIndex int

	// ------- replicated state -------
	storage map[int]map[string]string // actual kv-store
	latestOp map[int64]int            // client ID for duplicate detection

	config shardmaster.Config         // current configuration

	waitingFrom map[int][]string
	toTransfer []ShardTransfer
}

func StartServer(servers []*labrpc.ClientEnd, me int, persister *raft.Persister, maxraftstate int, gid int, masters []*labrpc.ClientEnd, make_end func(string) *labrpc.ClientEnd) *ShardKV {
	labgob.Register(Op{})
	labgob.Register(GetArgs{})
	labgob.Register(GetReply{})
	labgob.Register(PutAppendArgs{})
	labgob.Register(PutAppendReply{})
	labgob.Register(UpdateConfigArgs{})
	labgob.Register(UpdateConfigReply{})
	labgob.Register(FetchShardArgs{})
	labgob.Register(FetchShardReply{})
	labgob.Register(DeleteTransferArgs{})
	labgob.Register(DeleteTransferReply{})
	labgob.Register(ShardTransfer{})
	labgob.Register(CommitShardTransferArgs{})

	// fmt.Printf("NUMBER OF RUNNING GOROUTINES: %v\n", runtime.NumGoroutine())
	// pprof.Lookup("goroutine").WriteTo(os.Stdout, 1)

	kv := new(ShardKV)
	kv.me = me
	kv.maxraftstate = maxraftstate
	kv.make_end = make_end
	kv.gid = gid
	kv.masters = masters
	kv.persister = persister

	kv.applyCh = make(chan raft.ApplyMsg, 100000)
	kv.rf = raft.Make(servers, me, persister, kv.applyCh)
	kv.rs = raftservice.StartService(kv.rf, kv.dispatch, kv.dispatchControl, kv.applyCh)
	kv.mck = shardmaster.MakeClerk(kv.masters)

	// empty config
	kv.config.Groups = map[int][]string{}

	// haven't received anything yet
	kv.waitingFrom = map[int][]string{}

	kv.toTransfer = []ShardTransfer{}

	// storage variables
	kv.latestOp = map[int64]int{}
	kv.storage = map[int]map[string]string{}

	kv.active = true

	kv.lastIncludedIndex = 0

	// restore previous state if exists
	kv.readPersist(persister.ReadSnapshot())

	if kv.maxraftstate != -1 {
		go kv.snapshotLoop()
	}

	go kv.pollShardConfig()
	go kv.fetchLoop()
	return kv
}

func (kv *ShardKV) Kill() {
	kv.rf.Kill()
	kv.rs.Kill()
	kv.mu.Lock()
	defer kv.mu.Unlock()
	kv.active = false
}

func (kv *ShardKV) isActive() bool {
	kv.mu.Lock()
	defer kv.mu.Unlock()
	return kv.active
}

func (kv *ShardKV) SIDServedByMe(sid int) bool {
	return kv.config.Shards[sid] == kv.gid
}

func (kv *ShardKV) keyServedByMe(key string) bool {
	sid := key2shard(key)
	return kv.SIDServedByMe(sid)
}

func (kv *ShardKV) lockAndkeyServedByMe(key string) bool {
	kv.mu.Lock()
	defer kv.mu.Unlock()
	return kv.keyServedByMe(key)
}

func (kv *ShardKV) SIDReceived(sid int) bool {
	if _, ok := kv.waitingFrom[sid]; !ok {
		return true
	}

	return false
}

func (kv *ShardKV) receivedKey(key string) bool {
	sid := key2shard(key)
	return kv.SIDReceived(sid)
}

func (kv *ShardKV) lockAndreceivedKey(key string) bool {
	kv.mu.Lock()
	defer kv.mu.Unlock()
	return kv.receivedKey(key)
}

func (kv *ShardKV) isTransitioning() bool {
	kv.mu.Lock()
	defer kv.mu.Unlock()
	return len(kv.waitingFrom) > 0
}

func (kv *ShardKV) isTransitioningNoLock() bool {
	return len(kv.waitingFrom) > 0
}

// assume kv is locked by caller...
func (kv *ShardKV) storeTransfer(args ShardTransfer) {
	for _, transfer := range kv.toTransfer {
		if transfer.SID == args.SID && transfer.CID == args.CID && transfer.GID == args.GID {
			return
		}
	}

	kv.toTransfer = append(kv.toTransfer, args)
}

// assume kv is locked by caller...
func (kv *ShardKV) retrieveTransfer(sid int, cid int, gid int) (ShardTransfer, bool) {
	for _, transfer := range kv.toTransfer {
		if transfer.SID == sid && transfer.CID == cid && transfer.GID == gid {
			return transfer, true
		}
	}

	return ShardTransfer{}, false
}

// assume kv is locked by caller...
func (kv *ShardKV) deleteTransfer(args DeleteTransferArgs) {
	for ii, transfer := range kv.toTransfer {
		if transfer.SID == args.SID && transfer.CID == args.CID && transfer.GID == args.GID {
			kv.toTransfer[ii] = kv.toTransfer[len(kv.toTransfer) - 1]
			kv.toTransfer = kv.toTransfer[:len(kv.toTransfer) - 1]
		}
	}
}

// assume kv is locked by caller...
func (kv *ShardKV) DPrintf(format string, a ...interface{}) {
	if !kv.active { return }

	// if kv.me != 0 { return }

	serving := make([]int, 0)
	for sid, _ := range kv.config.Shards {
		if kv.SIDServedByMe(sid) && kv.SIDReceived(sid) {
			serving = append(serving, sid)
		}
	}
	sort.Ints(serving)

	waiting := make([]int, 0)
	for sid, _ := range kv.config.Shards {
		if kv.SIDServedByMe(sid) && !kv.SIDReceived(sid) {
			waiting = append(waiting, sid)
		}
	}
	sort.Ints(waiting)

	args := make([]interface{}, 0)
	args = append(args, kv.gid)
	args = append(args, kv.me)
	args = append(args, kv.config.Num)
	args = append(args, serving)
	args = append(args, waiting)
	args = append(args, a...)

	util.DPrintf(DEBUG, "SKV [%d:%d@conf-%d; serv:%v wait:%v] " + format, args...)
}

func (kv *ShardKV) LockAndDPrintf(format string, a ...interface{}) {
	kv.mu.Lock()
	defer kv.mu.Unlock()
	kv.DPrintf(format, a...)
}

func (kv *ShardKV) pollShardConfig() {
	for {
		if !kv.isActive() { return }
		kv.pollShardConfigOnce()
		time.Sleep(POLL_PERIOD)
	}
}

func (kv *ShardKV) pollShardConfigOnce() {
	if _, isLeader := kv.rf.GetState(); !isLeader { return }
	if !kv.isActive() { return }
	if kv.isTransitioning() { return }

	kv.mu.Lock()
	currentConfigNum := kv.config.Num
	kv.mu.Unlock()

	// query clerk for next config
	// clerk := shardmaster.MakeClerk(kv.masters)
	// nextConfig := clerk.Query(currentConfigNum + 1)

	// nextConfig := kv.mck.Query(currentConfigNum + 1)
	nextConfig, ok := kv.mck.QueryOnce(currentConfigNum + 1)

	if !ok {
		kv.LockAndDPrintf("failed to query master")
		return
	}

	if !kv.isActive() { return }
	if kv.isTransitioning() { return }

	kv.mu.Lock()

	if nextConfig.Num <= kv.config.Num {
		kv.mu.Unlock()
		return
	}

	kv.DPrintf("applying new config %d -> %d", kv.config.Num, nextConfig.Num)
	kv.mu.Unlock()

	// clerk := MakeClerk(kv.masters, kv.make_end)
	// clerk.UpdateConfig(kv.gid, nextConfig)
	kv.UpdateConfig(nextConfig)
}


///////////////////////////////////////////////////////////////////
/////////////////////////// RPC HANDLERS //////////////////////////
///////////////////////////////////////////////////////////////////

type Op struct {
	Typ  string
	Args interface{}
}

type GetResult struct {
	Err   string
	Value string
}

type PutAppendResult struct {
	Err string
}

func (kv *ShardKV) Get(args *GetArgs, reply *GetReply) {
	if !kv.isActive() { return }

	uid := id(args.ClientId, args.Id)
	op := Op{ "Get", *args }
	error, res := kv.rs.Start(uid, op)

	reply.WrongLeader = (error == raftservice.WrongLeaderErr)
	reply.Err = error

	if error == raftservice.OK {
		result, ok := res.(GetResult)
		util.Assert(ok)
		reply.Err = result.Err
		reply.Value = result.Value
	}
}

func (kv *ShardKV) PutAppend(args *PutAppendArgs, reply *PutAppendReply) {
	if !kv.isActive() { return }

	uid := id(args.ClientId, args.Id)
	op := Op{ "PutAppend", *args }
	error, res := kv.rs.Start(uid, op)

	reply.WrongLeader = (error == raftservice.WrongLeaderErr)
	reply.Err = error

	if error == raftservice.OK {
		result, ok := res.(PutAppendResult)
		util.Assert(ok)
		reply.Err = result.Err
	}
}


///////////////////////////////////////////////////////////////////
////////////////////// OTHER RAFT LOG SENDERS /////////////////////
///////////////////////////////////////////////////////////////////

func (kv *ShardKV) UpdateConfig(config shardmaster.Config) {
	if !kv.isActive() { return }

	kv.mu.Lock()
	kv.DPrintf("incoming UpdateConfig RPC call: %+v", config)
	currentConfigNum := kv.config.Num
	kv.mu.Unlock()

	if config.Num <= currentConfigNum { return }

	clientId := id(kv.gid, kv.me)
	uid := id(clientId, config.Num)
	op := Op{ "UpdateConfig", UpdateConfigArgs{ config, clientId, config.Num } }
	error, _ := kv.rs.Start(uid, op)

	kv.mu.Lock()
	kv.DPrintf("FINISHED incoming UpdateConfig RPC call, error = %v", error)
	kv.mu.Unlock()
}

func (kv *ShardKV) fetchLoop() {
	for {
		if !kv.isActive() { return }
		kv.fetchAllMissingShards()
		time.Sleep(POLL_PERIOD)
	}
}

func (kv *ShardKV) fetchAllMissingShards() {
	if _, isLeader := kv.rf.GetState(); !isLeader { return }

	kv.mu.Lock()
	for sid, _ := range kv.waitingFrom {
		util.Assert(len(kv.waitingFrom[sid]) > 0)
		go kv.fetchMissingShard(sid)
	}
	kv.mu.Unlock()
}

func (kv *ShardKV) fetchMissingShard(sid int) {
	if !kv.isActive() { return }

	kv.mu.Lock()
	args := FetchShardArgs{}
	args.GID = kv.gid
	args.SID = sid
	args.CID = kv.config.Num

	servers, ok := kv.waitingFrom[sid]
	kv.mu.Unlock()

	if !ok {
		kv.LockAndDPrintf("seems like someone fetched sid#%v in the meantime...", sid)
		return
	}

	util.Assert(len(servers) > 0)

	for _, server := range servers {
		srv := kv.make_end(server)
		var reply FetchShardReply
		ok := srv.Call("ShardKV.FetchShard", &args, &reply)

		if ok && reply.WrongLeader == false && reply.Err == OK {
			kv.commitShardTransfer(reply.Transfer)
			return
		}

		kv.LockAndDPrintf("fetchMissingShard failed once! args = %+v, servers = %+v, reply = %+v", args, servers, reply)
	}

	kv.LockAndDPrintf("fetchMissingShard failed! args = %+v, servers = %+v", args, servers)
}

func (kv *ShardKV) FetchShard(args *FetchShardArgs, reply *FetchShardReply) {
	if !kv.isActive() { return }

	kv.mu.Lock()
	kv.DPrintf("incoming FetchShard RPC call: %+v", args)
	transfer, ok := kv.retrieveTransfer(args.SID, args.CID, args.GID)
	kv.mu.Unlock()

	reply.WrongLeader = false
	reply.Err = raftservice.OK

	if _, isLeader := kv.rf.GetState(); !isLeader {
		reply.WrongLeader = true
		reply.Err = raftservice.WrongLeaderErr
		return
	}

	// TODO: copy

	if !ok {
		reply.Err = "Not have yet!"
		return
	}

	reply.Transfer = transfer

	kv.mu.Lock()
	kv.DPrintf("FINISHED incoming FetchShard RPC call, reply = %v", reply)
	kv.mu.Unlock()
}

func (kv *ShardKV) DeleteTransfer(args *DeleteTransferArgs, reply *DeleteTransferReply) {
	if !kv.isActive() { return }

	uid := id(args.ClientId, args.Id)
	op := Op{ "DeleteTransfer", *args }
	error, _ := kv.rs.Start(uid, op)

	reply.WrongLeader = (error == raftservice.WrongLeaderErr)
	reply.Err = error
}

func (kv *ShardKV) commitShardTransfer(transfer ShardTransfer) {
	if !kv.isActive() { return }

	kv.mu.Lock()
	kv.DPrintf("incoming commitShardTransfer RPC call: %+v", transfer)
	kv.mu.Unlock()

	// TODO: do some checks here?

	clientId := id(kv.gid, kv.me)
	myId := transfer.SID // TODO
	uid := id(clientId, myId)
	op := Op{ "commitShardTransfer", CommitShardTransferArgs{ transfer, clientId, myId } }
	error, _ := kv.rs.Start(uid, op)

	kv.mu.Lock()
	kv.DPrintf("FINISHED incoming UpdateConfig RPC call, error = %v", error)
	kv.mu.Unlock()
}


///////////////////////////////////////////////////////////////////
//////////////////////// COMMAND EXECUTION ////////////////////////
///////////////////////////////////////////////////////////////////

func (kv *ShardKV) dispatch(msg raft.ApplyMsg) (string, interface{}) {
	kv.mu.Lock()
	defer kv.mu.Unlock()

	op, ok := msg.Command.(Op)
	util.Assert(ok)

	index := msg.CommandIndex
	util.Assert(index == kv.lastIncludedIndex + 1, "index = %d, kv.lastIncludedIndex = %d", index, kv.lastIncludedIndex)
	kv.lastIncludedIndex = index

	switch args := op.Args.(type) {
	case GetArgs          : return kv.applyGet(msg.CommandIndex, args)
	case PutAppendArgs    : return kv.applyPutAppend(msg.CommandIndex, args)
	case UpdateConfigArgs : return kv.applyUpdateConfig(msg.CommandIndex, args)
	case CommitShardTransferArgs: return kv.applyCommitShardTransfer(msg.CommandIndex, args)
	case DeleteTransferArgs: return kv.applyDeleteTransfer(msg.CommandIndex, args)
	default               : panic("Unknown argument type in dispatch")
	}
}

func (kv *ShardKV) applyGet(index int, args GetArgs) (string, interface{}) {
	kv.DPrintf("applyGet(%d, %+v)", index, args)

	if !kv.keyServedByMe(args.Key) {
		kv.DPrintf("rejecting Get: shard moved")
		return id(args.ClientId, args.Id), GetResult{ ErrWrongGroup, "" }
	}

	// kv.DPrintf("!!!!!!!CONFIG %+v", kv.config)
	// kv.DPrintf("!!!!!!!WAITING FROM %+v", kv.waitingFrom)

	if !kv.receivedKey(args.Key) {
		kv.DPrintf("rejecting Get: key has not been received yet")
		return id(args.ClientId, args.Id), GetResult{ WaitingForShardErr, "" }
	}

	result, ok := kv.storage[key2shard(args.Key)][args.Key]
	if !ok { result = "" }

	return id(args.ClientId, args.Id), GetResult{ OK, result }
}

func (kv *ShardKV) applyPutAppend(index int, args PutAppendArgs) (string, interface{}) {
	kv.DPrintf("applyPutAppend(%d, %+v)", index, args)

	if !kv.keyServedByMe(args.Key) {
		kv.DPrintf("rejecting PutAppend: shard moved")
		return id(args.ClientId, args.Id), PutAppendResult{ ErrWrongGroup }
	}

	if !kv.receivedKey(args.Key) {
		kv.DPrintf("rejecting PutAppend: key has not been received yet")
		return id(args.ClientId, args.Id), PutAppendResult{ WaitingForShardErr }
	}

	sid := key2shard(args.Key)

	// lazily allocate storage for shard
	if _, ok := kv.storage[sid]; !ok {
		kv.storage[sid] = map[string]string{}
	}

	if args.Id > kv.latestOp[args.ClientId] {
		switch args.Op {
		case "Put"	 : kv.storage[sid][args.Key]  = args.Value
		case "Append": kv.storage[sid][args.Key] += args.Value
		default      : panic("Unknown PutAppend Operation")
		}

		kv.latestOp[args.ClientId] = args.Id
	}

	// kv.lastIncludedIndex = index
	return id(args.ClientId, args.Id), PutAppendResult{ OK }
}

func (kv *ShardKV) applyUpdateConfig(index int, args UpdateConfigArgs) (string, interface{}) {
	kv.DPrintf("applyUpdateConfig(%d, %+v)", index, args)
	// defer func() { go kv.pollShardConfigOnce() }()

	if args.Config.Num <= kv.config.Num {
		kv.DPrintf("not applying config; out-of-date")
		return id(args.ClientId, args.Id), nil
	}

	if kv.isTransitioningNoLock() {
		kv.DPrintf("not applying config; transitioning")
		return id(args.ClientId, args.Id), nil
	}

	lostShards := []int{}
	gainedShards := []int{}

	for sid, _ := range kv.config.Shards {
		if (kv.config.Shards[sid] == kv.gid) && (args.Config.Shards[sid] != kv.gid) {
			lostShards = append(lostShards, sid)
		}

		if (kv.config.Shards[sid] != kv.gid) && (args.Config.Shards[sid] == kv.gid) {
			gainedShards = append(gainedShards, sid)
		}
	}

	// defensively make sure order is deterministic
	sort.Ints(lostShards)
	sort.Ints(gainedShards)

	kv.DPrintf("lost = %v", lostShards)
	kv.DPrintf("gained = %v", gainedShards)

	// send lost shards
	for _, sid := range lostShards {
		toGID := args.Config.Shards[sid]
		if toGID == UNASSIGNED { continue }

		transferArgs := ShardTransfer{}
		transferArgs.GID       = toGID
		transferArgs.SID       = sid
		transferArgs.CID       = args.Config.Num
		transferArgs.Shard     = kv.cloneShard(sid)
		transferArgs.LatestOps = kv.cloneLatestOps()

		kv.storeTransfer(transferArgs)
	}

	// clean up shards
	for _, sid := range lostShards {
		util.Assert(len(kv.waitingFrom[sid]) == 0)
		delete(kv.storage, sid)
	}

	// wait for gained shards
	for _, sid := range gainedShards {
		if kv.config.Shards[sid] == UNASSIGNED {
			util.Assert(len(kv.waitingFrom[sid]) == 0)
			continue // no need to wait; start serving right away!
		}

		fromGID := kv.config.Shards[sid]
		kv.waitingFrom[sid] = kv.config.Groups[fromGID] // save here cause new config might not have it
	}

	kv.config = cloneConfig(args.Config)
	return id(args.ClientId, args.Id), nil
}

func max(a int, b int) int {
	if a > b { return a } else { return b }
}

func (kv *ShardKV) applyCommitShardTransfer(index int, args CommitShardTransferArgs) (string, interface{}) {
	kv.DPrintf("applyCommitShardTransfer(%d, %+v)", index, args)

	util.Assert(args.Transfer.GID == kv.gid)
	util.Assert(args.Transfer.CID <= kv.config.Num)

	if args.Transfer.CID < kv.config.Num {
		kv.DPrintf("not applying transfer; out-of-date")
		return id(args.ClientId, args.Id), nil
	}

	if kv.SIDReceived(args.Transfer.SID) {
		util.Assert(len(kv.waitingFrom[args.Transfer.SID]) == 0)
		kv.DPrintf("not applying transfer; already received")
		return id(args.ClientId, args.Id), nil
	}

	util.Assert(len(kv.waitingFrom[args.Transfer.SID]) > 0)

	delete(kv.storage, args.Transfer.SID)

	kv.storage[args.Transfer.SID] = map[string]string{}

	for k, v := range args.Transfer.Shard {
		kv.storage[args.Transfer.SID][k] = v
	}

	for client, _ := range args.Transfer.LatestOps {
		kv.latestOp[client] = max(kv.latestOp[client], args.Transfer.LatestOps[client])
	}



// type DeleteTransferArgs struct {
// 	GID      int
// 	SID      int
// 	CID      int
// 	ClientId int64
// 	Id       int
// }

	go func(transfer ShardTransfer, servers []string){
		args := DeleteTransferArgs{}
		args.GID = transfer.GID
		args.SID = transfer.SID
		args.CID = transfer.CID
		args.ClientId = nrand()
		args.Id = 1
		// ...

		for _, server := range servers {
			srv := kv.make_end(server)
			var reply DeleteTransferReply
			ok := srv.Call("ShardKV.DeleteTransfer", &args, &reply)

			if ok && reply.WrongLeader == false && reply.Err == OK {
				return
			}
		}

		kv.LockAndDPrintf("deleteTransfer failed! args = %+v, servers = %+v", args, servers)
	}(args.Transfer, kv.waitingFrom[args.Transfer.SID])

	delete(kv.waitingFrom, args.Transfer.SID)

	return id(args.ClientId, args.Id), nil
}

func (kv *ShardKV) applyDeleteTransfer(index int, args DeleteTransferArgs) (string, interface{}) {
	kv.DPrintf("applyDeleteTransfer(%d, %+v)", index, args)

	kv.deleteTransfer(args)

	return id(args.ClientId, args.Id), nil
}


///////////////////////////////////////////////////////////////////
//////////////////////// CONTROL MESSAGES /////////////////////////
///////////////////////////////////////////////////////////////////

func (kv *ShardKV) dispatchControl(msg raft.ApplyMsg) {
	switch args := msg.Command.(type) {
	case []byte: kv.applySnapshot(args)
	default    : panic("Unknown argument type in dispatchControl")
	}
}

func (kv *ShardKV) applySnapshot(snapshot []byte) {
	kv.mu.Lock()
	defer kv.mu.Unlock()
	kv.DPrintf("applying snapshot %+v", snapshot)
	kv.readPersist(snapshot)
}

///////////////////////////////////////////////////////////////////
///////////////////////////// HELPERS /////////////////////////////
///////////////////////////////////////////////////////////////////

// assume locked ...
func (kv *ShardKV) cloneShard(sid int) map[string]string {
	new := map[string]string{}
	for k, v := range kv.storage[sid] { new[k] = v }
	return new
}

// assume locked ...
func (kv *ShardKV) cloneLatestOps() map[int64]int {
	new := map[int64]int{}
	for k, v := range kv.latestOp { new[k] = v }
	return new
}

func cloneConfig(old shardmaster.Config) shardmaster.Config {
	new := shardmaster.Config{}
	new.Num = old.Num
	new.Shards = [shardmaster.NShards]int{}
	new.Groups = map[int][]string{}

	// copy old groups
	for k, v := range old.Groups {
		new.Groups[k] = v
	}

	// copy old shards
	for sid, gid := range old.Shards {
		new.Shards[sid] = gid
	}

	return new
}










func (kv *ShardKV) snapshotLoop() {
	for {
		if !kv.isActive() { return }

		kv.trySnapshot()
		time.Sleep(SLEEP_PERIOD)
	}
}

func (kv *ShardKV) trySnapshot() {
	kv.mu.Lock()
	threshold := int(float64(kv.maxraftstate) * 0.95)
	if kv.maxraftstate != -1 && kv.persister.RaftStateSize() > threshold {
		kv.DPrintf("creating snapshot")
		snapshot := kv.serialize()
		lastIncludedIndex := kv.lastIncludedIndex
		kv.mu.Unlock()
		kv.rf.Snapshot(lastIncludedIndex, snapshot)
	} else {
		kv.mu.Unlock()
	}
}

// assume kv locked...
func (kv *ShardKV) serialize() []byte {
	w := new(bytes.Buffer)
	e := labgob.NewEncoder(w)

	e.Encode(kv.lastIncludedIndex)
	e.Encode(kv.storage)
	e.Encode(kv.latestOp)
	e.Encode(kv.config)
	e.Encode(kv.waitingFrom)
	e.Encode(kv.toTransfer)

	return w.Bytes()
}

// assume locked
func (kv *ShardKV) readPersist(data []byte) {
	if data == nil || len(data) < 1 {
		return
	}

	r := bytes.NewBuffer(data)
	d := labgob.NewDecoder(r)

	var lastIncludedIndex int
	var storage map[int]map[string]string
	var latestOp map[int64]int
	var config shardmaster.Config
	var waitingFrom map[int][]string
	var toTransfer []ShardTransfer

	if err := d.Decode(&lastIncludedIndex); err != nil {
		log.Fatal("Unable to deserialize lastIncludedIndex", err)
	}

	if err := d.Decode(&storage); err != nil {
		log.Fatal("Unable to deserialize storage", err)
	}

	if err := d.Decode(&latestOp); err != nil {
		log.Fatal("Unable to deserialize latestOp", err)
	}

	if err := d.Decode(&config); err != nil {
		log.Fatal("Unable to deserialize config", err)
	}

	if err := d.Decode(&waitingFrom); err != nil {
		log.Fatal("Unable to deserialize waitingFrom", err)
	}

	if err := d.Decode(&toTransfer); err != nil {
		log.Fatal("Unable to deserialize toTransfer", err)
	}

	kv.lastIncludedIndex = lastIncludedIndex
	kv.storage = storage
	kv.latestOp = latestOp
	kv.config = config
	kv.waitingFrom = waitingFrom
	kv.toTransfer = toTransfer
}