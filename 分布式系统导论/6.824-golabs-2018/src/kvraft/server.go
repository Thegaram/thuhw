package raftkv

import "bytes"
import "labgob"
import "labrpc"
import "log"
import "raft"
import "substore"
import "sync"
import "time"

const CONSENSUS_TIMEOUT = 5 * 100 * time.Millisecond
const SLEEP_PERIOD 		= 10      * time.Millisecond


type Op struct {
	Typ string
	Args interface{}
}

type Notification = struct{Op; string}


type KVServer struct {
	mu      sync.Mutex
	me      int
	rf      *raft.Raft
	applyCh chan raft.ApplyMsg

	persister *raft.Persister

	maxraftstate int // snapshot if log grows this big

	subs *substore.SubscriptionStore

	// replicated state
	// transitions: ApplyMsg from Raft
	// make sure to lock around changes! (atomic changes)
	latestOpForClient map[int64]int
	storage map[string]string
	lastIncludedIndex int

	active bool
}

// assume rf is locked by caller...
func (kv *KVServer) DPrintf(format string, a ...interface{}) {
	if !kv.active { return }

	args := make([]interface{}, 0)
	args = append(args, kv.me)
	args = append(args, kv.lastIncludedIndex)
	args = append(args, kv.persister.RaftStateSize())
	args = append(args, a...)

	DPrintf("KVSE [%d@%2d, %d] " + format, args...)
}

func (kv *KVServer) LockAndDPrintf(format string, a ...interface{}) {
	kv.mu.Lock()
	defer kv.mu.Unlock()
	kv.DPrintf(format, a...)
}

func (kv *KVServer) isActive() bool {
	kv.mu.Lock()
	defer kv.mu.Unlock()
	return kv.active
}

func (kv *KVServer) GetRPC(args *GetArgs, reply *GetReply) {

	if !kv.isActive() {
		return
	}

	op := Op { "Get", *args }
	index, _, isLeader := kv.rf.Start(op)

	if !isLeader {
		reply.WrongLeader = true
		return
	}

	sub := kv.subs.Subscribe(index)
	defer kv.subs.Release(index)

	select {
	case res := <-sub:
		not, ok := res.(Notification)
		assert(ok)

		if op != not.Op {
			reply.Err = "Concurrent request won!"
			return
		}

		reply.Err = OK
		reply.Value = not.string

	case <-time.After(CONSENSUS_TIMEOUT):
		// kv.LockAndDPrintf("consensus timeout for GetRPC %+v", args)
		reply.Err = "Timeout"
		return
	}

}

func (kv *KVServer) PutAppendRPC(args *PutAppendArgs, reply *PutAppendReply) {

	if !kv.isActive() {
		return
	}

	op := Op { "PutAppend", *args }
	index, _, isLeader := kv.rf.Start(op)

	if !isLeader {
		reply.WrongLeader = true
		return
	}

	sub := kv.subs.Subscribe(index)
	defer kv.subs.Release(index)

	select {
	case res := <-sub:
		not, ok := res.(Notification)
		assert(ok)

		if op != not.Op {
			reply.Err = "Concurrent request won!"
			return
		}

		reply.Err = OK

	case <-time.After(CONSENSUS_TIMEOUT):
		// kv.LockAndDPrintf("consensus timeout for PutAppendRPC %+v", args)
		reply.Err = "Timeout"
		return
	}

}

func (kv *KVServer) Kill() {
	kv.rf.Kill()
	kv.mu.Lock()
	defer kv.mu.Unlock()
	kv.active = false
}

func StartKVServer(servers []*labrpc.ClientEnd, me int, persister *raft.Persister, maxraftstate int) *KVServer {
	labgob.Register(Op{})
	labgob.Register(PutAppendArgs{})
	labgob.Register(GetArgs{})

	kv := new(KVServer)
	kv.me = me
	kv.maxraftstate = maxraftstate

	kv.persister = persister

	kv.applyCh = make(chan raft.ApplyMsg, 1024)
	kv.rf = raft.Make(servers, me, persister, kv.applyCh)

	kv.subs = substore.MakeSubStore()

	kv.latestOpForClient = make(map[int64]int)
	kv.storage = make(map[string]string)
	kv.lastIncludedIndex = 0

	kv.active = true

	// restore previous state if exists
	kv.readPersist(persister.ReadSnapshot())

	go kv.applierLoop()

	if kv.maxraftstate != -1 {
		go kv.snapshotLoop()
	}

	return kv
}

func (kv *KVServer) snapshotLoop() {
	for {
		if (!kv.isActive()) {
			return
		}

		kv.trySnapshot()
		time.Sleep(SLEEP_PERIOD)
	}
}

func (kv *KVServer) trySnapshot() {
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
func (kv *KVServer) serialize() []byte {
	w := new(bytes.Buffer)
	e := labgob.NewEncoder(w)

	e.Encode(kv.lastIncludedIndex)
	e.Encode(kv.storage)
	e.Encode(kv.latestOpForClient)

	return w.Bytes()
}

// assume locked
func (kv *KVServer) readPersist(data []byte) {
	if data == nil || len(data) < 1 {
		return
	}

	r := bytes.NewBuffer(data)
	d := labgob.NewDecoder(r)

	var lastIncludedIndex int
	var storage map[string]string
	var latestOpForClient map[int64]int

	if err := d.Decode(&lastIncludedIndex); err != nil {
		log.Fatal("Unable to deserialize lastIncludedIndex", err)
	}

	if err := d.Decode(&storage); err != nil {
		log.Fatal("Unable to deserialize storage", err)
	}

	if err := d.Decode(&latestOpForClient); err != nil {
		log.Fatal("Unable to deserialize latestOpForClient", err)
	}

	kv.lastIncludedIndex = lastIncludedIndex
	kv.storage = storage
	kv.latestOpForClient = latestOpForClient
}

func (kv *KVServer) applierLoop() {
	for msg := range kv.applyCh {
		if !kv.isActive() {
			return
		}

		kv.LockAndDPrintf("got applyMsg: %+v", msg)
		if (msg.CommandValid){
			kv.dispathKVCommand(msg)
		} else {
			kv.dispathControlCommand(msg)
		}
		kv.LockAndDPrintf("finished applying: %+v", msg)

		// NOTE: do not snapshot here; that way the
		// kvserver will not be able to keep up with
		// incoming applyMsgs
	}
}

func (kv *KVServer) dispathKVCommand(msg raft.ApplyMsg) {
	index := msg.CommandIndex
	op, ok := msg.Command.(Op)

	if !ok {
		panic("Invalid command received: conversion to Op failed")
	}

	switch args := op.Args.(type) {
	case GetArgs          : kv.applyGet(index, op, args)
	case PutAppendArgs    : kv.applyPutAppend(index, op, args)
	default               : log.Fatal("Unknown argument type in dispathKVCommand")
	}
}

func (kv *KVServer) applyGet(index int, op Op, args GetArgs) {
	kv.mu.Lock()
	defer kv.mu.Unlock()
	kv.DPrintf("applyGet(%d, %+v, %+v)", index, op, args)

	assert(op.Typ == "Get")
	assert(index == kv.lastIncludedIndex + 1, "index = %d, kv.lastIncludedIndex = %d", index, kv.lastIncludedIndex)

	value, ok := kv.storage[args.Key]
	if !ok { value = "" }

	kv.lastIncludedIndex = index
	kv.subs.NotifyAll(index, Notification{op, value})
}

func (kv *KVServer) applyPutAppend(index int, op Op, args PutAppendArgs) {
	kv.mu.Lock()
	defer kv.mu.Unlock()
	kv.DPrintf("applyPutAppend(%d, %+v, %+v)", index, op, args)

	assert(op.Typ == "PutAppend")
	assert(index == kv.lastIncludedIndex + 1, "index = %d, kv.lastIncludedIndex = %d", index, kv.lastIncludedIndex)

	if args.Id > kv.latestOpForClient[args.ClientId] {
		switch args.Op {
		case "Put"	 : kv.storage[args.Key]  = args.Value
		case "Append": kv.storage[args.Key] += args.Value
		default      : log.Fatal("Unknown PutAppend Operation", args.Op)
		}

		kv.latestOpForClient[args.ClientId] = args.Id
	}

	kv.lastIncludedIndex = index
	kv.subs.NotifyAll(index, Notification{op, ""})
}

func (kv *KVServer) dispathControlCommand(msg raft.ApplyMsg) {
	switch args := msg.Command.(type) {
	case []byte: kv.applySnapshot(args)
	default    : log.Fatal("Unknown argument type in dispathControlCommand")
	}
}

func (kv *KVServer) applySnapshot(snapshot []byte) {
	kv.mu.Lock()
	defer kv.mu.Unlock()
	kv.DPrintf("applying snapshot %+v", snapshot)
	kv.readPersist(snapshot)
}