package shardmaster

import "labgob"
import "labrpc"
import "raft"
import "raftservice"
import "sort"
import "sync"
import "util"
import "fmt"

// enable debug output
const DEBUG = false

// GID reserved for unassigned shards
const INVALID_GID = 0

type Op struct {
	Typ string
	Args interface{}
}

// create unique ID from ClientId and RequestId
func id(components ...interface{}) string {
    result := ""

    for ii, c := range components {
        if ii > 0 { result += "-" }
        result += fmt.Sprintf("%v", c)
    }

    return result
}

type ShardMaster struct {
	mu      sync.Mutex
	me      int
	rf      *raft.Raft

	// service taking care of interaction with Raft
	rs *raftservice.RaftService

	// latest, monotonically increasing index from client
	latestOp map[int64]int

	lastCID int
	configs []Config // indexed by CID

	active bool
}

func StartServer(servers []*labrpc.ClientEnd, me int, persister *raft.Persister) *ShardMaster {
	labgob.Register(Op{})
	labgob.Register(Config{})
	labgob.Register(JoinArgs{})
	labgob.Register(JoinReply{})
	labgob.Register(LeaveArgs{})
	labgob.Register(LeaveReply{})
	labgob.Register(MoveArgs{})
	labgob.Register(MoveReply{})
	labgob.Register(QueryArgs{})
	labgob.Register(QueryReply{})

	sm := new(ShardMaster)
	sm.me = me

	applyCh := make(chan raft.ApplyMsg)
	sm.rf = raft.Make(servers, me, persister, applyCh)
	sm.rs = raftservice.StartService(sm.rf, sm.dispath, sm.dispathControl, applyCh)

	sm.latestOp = make(map[int64]int)
	sm.lastCID = 0

	sm.configs = make([]Config, 1)
	sm.configs[0].Groups = map[int][]string{}

	sm.active = true
	return sm
}

func (sm *ShardMaster) Raft() *raft.Raft {
	return sm.rf
}

func (sm *ShardMaster) Kill() {
	sm.rf.Kill()
	sm.rs.Kill()
	sm.mu.Lock()
	defer sm.mu.Unlock()
	sm.active = false
}

func (sm *ShardMaster) isActive() bool {
	sm.mu.Lock()
	defer sm.mu.Unlock()
	return sm.active
}

// assume sm is locked by caller...
func (sm *ShardMaster) DPrintf(format string, a ...interface{}) {
	if !sm.active { return }

	// if sm.me != 0 { return }

	args := make([]interface{}, 0)
	args = append(args, sm.me)
	args = append(args, a...)

	util.DPrintf(DEBUG, "SM [%d] " + format, args...)
}


///////////////////////////////////////////////////////////////////
/////////////////////////// RPC HANDLERS //////////////////////////
///////////////////////////////////////////////////////////////////

func (sm *ShardMaster) Join(args *JoinArgs, reply *JoinReply) {
	if !sm.isActive() { return }
	error, _ := sm.rs.Start(id(args.ClientId, args.Id), Op{ "Join", *args })
	reply.WrongLeader = (error == raftservice.WrongLeaderErr)
	reply.Err = error
}

func (sm *ShardMaster) Leave(args *LeaveArgs, reply *LeaveReply) {
	if !sm.isActive() { return }
	error, _ := sm.rs.Start(id(args.ClientId, args.Id), Op{ "Leave", *args })
	reply.WrongLeader = (error == raftservice.WrongLeaderErr)
	reply.Err = error
}

func (sm *ShardMaster) Move(args *MoveArgs, reply *MoveReply) {
	if !sm.isActive() { return }
	error, _ := sm.rs.Start(id(args.ClientId, args.Id), Op{ "Move", *args })
	reply.WrongLeader = (error == raftservice.WrongLeaderErr)
	reply.Err = error
}

func (sm *ShardMaster) Query(args *QueryArgs, reply *QueryReply) {
	if !sm.isActive() { return }

	sm.mu.Lock()
	sm.DPrintf("PREPARING QUERY: %+v", args)
	sm.mu.Unlock()

	error, res := sm.rs.Start(id(args.ClientId, args.Id), Op{ "Query", *args })
	sm.DPrintf("FINISHED QUERY11111: %+v, %+v", error, res)
	reply.WrongLeader = (error == raftservice.WrongLeaderErr)
	reply.Err = error
	sm.DPrintf("FINISHED QUERY22222: %+v, %+v", args, reply)

	if error == raftservice.OK {
		config, ok := res.(Config)
		util.Assert(ok)
		reply.Config = cloneConfig(config)
	}

	sm.mu.Lock()
	sm.DPrintf("FINISHED QUERY: %+v, %+v", args, reply)
	sm.mu.Unlock()
}


///////////////////////////////////////////////////////////////////
//////////////////////// COMMAND EXECUTION ////////////////////////
///////////////////////////////////////////////////////////////////

func (sm *ShardMaster) dispath(msg raft.ApplyMsg) (string, interface{}) {
	op, ok := msg.Command.(Op)
	util.Assert(ok)

	switch args := op.Args.(type) {
	case QueryArgs: return sm.applyQuery(msg.CommandIndex, args)
	case JoinArgs : return sm.applyJoin(msg.CommandIndex, args)
	case LeaveArgs: return sm.applyLeave(msg.CommandIndex, args)
	case MoveArgs : return sm.applyMove(msg.CommandIndex, args)
	default       : panic("Unknown argument type in dispath")
	}
}

func (sm *ShardMaster) applyQuery(index int, args QueryArgs) (string, interface{}) {
	sm.mu.Lock()
	defer sm.mu.Unlock()
	sm.DPrintf("applyQuery(%d, %+v)", index, args)

	cid := args.Num
	if cid <= -1 || cid > sm.lastCID { cid = sm.lastCID }
	config := cloneConfig(sm.configs[cid])

	return id(args.ClientId, args.Id), config
}

func (sm *ShardMaster) applyJoin(index int, args JoinArgs) (string, interface{}) {
	sm.mu.Lock()
	defer sm.mu.Unlock()
	sm.DPrintf("applyJoin(%d, %+v)", index, args)

	if args.Id > sm.latestOp[args.ClientId] {
		// clone last config
		conf := cloneConfig(sm.configs[sm.lastCID])
		sm.lastCID += 1
		conf.Num = sm.lastCID

		// add new groups
		for k, v := range args.Servers {
			conf.Groups[k] = v
		}

		sm.reassignShards(&conf)
		sm.configs = append(sm.configs, conf)
		sm.latestOp[args.ClientId] = args.Id
	}

	return id(args.ClientId, args.Id), nil
}

func (sm *ShardMaster) applyLeave(index int, args LeaveArgs) (string, interface{}) {
	sm.mu.Lock()
	defer sm.mu.Unlock()
	sm.DPrintf("applyLeave(%d, %+v), config: %+v", index, args, sm.configs[sm.lastCID])

	if args.Id > sm.latestOp[args.ClientId] {
		// clone last config
		conf := cloneConfig(sm.configs[sm.lastCID])
		sm.lastCID += 1
		conf.Num = sm.lastCID

		// remove groups
		for _, gid := range args.GIDs {
			delete(conf.Groups, gid)

			// unassign affected shards
			for sid, _ := range conf.Shards {
				if conf.Shards[sid] == gid {
					conf.Shards[sid] = INVALID_GID
				}
			}
		}

		sm.reassignShards(&conf)
		sm.configs = append(sm.configs, conf)
		sm.latestOp[args.ClientId] = args.Id

		sm.DPrintf("finished applyLeave(%d, %+v), config: %+v", index, args, sm.configs[sm.lastCID])
	}

	return id(args.ClientId, args.Id), nil
}

func (sm *ShardMaster) applyMove(index int, args MoveArgs) (string, interface{}) {
	sm.mu.Lock()
	defer sm.mu.Unlock()
	sm.DPrintf("applyMove(%d, %+v)", index, args)

	if args.Id > sm.latestOp[args.ClientId] {
		// clone last config
		conf := cloneConfig(sm.configs[sm.lastCID])
		sm.lastCID += 1
		conf.Num = sm.lastCID

		// move shard
		conf.Shards[args.Shard] = args.GID
		// NOTE: do not rebalance!

		sm.configs = append(sm.configs, conf)
		sm.latestOp[args.ClientId] = args.Id
	}

	return id(args.ClientId, args.Id), nil
}


///////////////////////////////////////////////////////////////////
//////////////////////// CONTROL MESSAGES /////////////////////////
///////////////////////////////////////////////////////////////////

func (sm *ShardMaster) dispathControl(msg raft.ApplyMsg) {
	panic("unexpected dispathControl call")
}


///////////////////////////////////////////////////////////////////
///////////////////////////// HELPERS /////////////////////////////
///////////////////////////////////////////////////////////////////

func cloneConfig(old Config) Config {
	new := Config{}
	new.Num = old.Num
	new.Shards = [NShards]int{}
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

func extractGroupsInConsistentOrder(config Config) []int {
	// get all GIDs with consistent order
	groups := make([]int, len(config.Groups))

	i := 0
	for gid, _ := range config.Groups {
		util.Assert(gid != INVALID_GID)
	    groups[i] = gid
	    i++
	}

	sort.Ints(groups)
	return groups
}

func findMinMaxAssigned(config Config) (int, int, int, int) {
	groups := extractGroupsInConsistentOrder(config)

	minGID := INVALID_GID; minAssigned := NShards + 1
	maxGID := INVALID_GID; maxAssigned := -1

	for _, gid := range groups {
		// count number of shards assigned to this GID
		count := 0
		for _, gid2 := range config.Shards {
			if gid2 == gid { count += 1 }
		}

		if count < minAssigned { minAssigned = count; minGID = gid }
		if count > maxAssigned { maxAssigned = count; maxGID = gid }
	}

	util.Assert(minGID != INVALID_GID)
	util.Assert(maxGID != INVALID_GID)
	util.Assert(maxAssigned >= minAssigned)

	return minGID, minAssigned, maxGID, maxAssigned
}

func (sm *ShardMaster) reassignShards(config *Config) {
	sm.DPrintf("reassignShards for config %+v", config)

	// no groups left => make sure to unassign everyone
	if len(config.Groups) == 0 {
		for sid, _ := range config.Shards {
			config.Shards[sid] = INVALID_GID
		}
		return
	}

	// assign unassigned shards
	for sid, oldGID := range config.Shards {
		if oldGID != INVALID_GID { continue }

		minGID, _, _, _ := findMinMaxAssigned(*config)
		sm.DPrintf("Assigning SID#%d to GID#%d", sid, minGID)
		config.Shards[sid] = minGID
	}

	// rebalance assigned shards
	for {
		minGID, minAssigned, maxGID, maxAssigned := findMinMaxAssigned(*config)
		if (maxAssigned - minAssigned) <= 1 { break }

		// reassign one
		for sid, gid := range config.Shards {
			if gid == maxGID {
				sm.DPrintf("Reassigning SID#%d from GID#%d to GID#%d", sid, maxGID, minGID)
				config.Shards[sid] = minGID
				break
			}
		}
	}

	sm.DPrintf("finished reassignShards for config %+v", config)
}