package shardkv

import "shardmaster"

//
// Sharded key/value server.
// Lots of replica groups, each running op-at-a-time paxos.
// Shardmaster decides which group serves each shard.
// Shardmaster may change shard assignment from time to time.
//
// You will have to modify these definitions.
//

const (
	OK             = "OK"
	ErrNoKey       = "ErrNoKey"
	ErrWrongGroup  = "ErrWrongGroup"
	WrongLeaderErr = "Wrong Leader"
	ConcurrentErr  = "Replaced by concurrent request"
	TimeoutErr     = "Timeout"
	WaitingForShardErr = "Waiting for shard"
)

type PutAppendArgs struct {
	Key      string
	Value    string
	Op       string // "Put" or "Append"
	ClientId int64
	Id       int
}

type PutAppendReply struct {
	WrongLeader bool
	Err         string
}

type GetArgs struct {
	Key      string
	ClientId int64
	Id       int
}

type GetReply struct {
	WrongLeader bool
	Err         string
	Value       string
}

type UpdateConfigArgs struct {
	Config   shardmaster.Config
	// ClientId int64
	ClientId string
	Id       int
}

type UpdateConfigReply struct {
	WrongLeader bool
	Err         string
}

type ShardTransfer struct {
	GID       int
	SID       int
	CID       int
	Shard     map[string]string
	LatestOps map[int64]int
}

type CommitShardTransferArgs struct {
	Transfer  ShardTransfer
	ClientId string
	Id       int
}

type FetchShardArgs struct {
	GID int
	SID int
	CID int
}

type FetchShardReply struct {
	Transfer    ShardTransfer
	WrongLeader bool
	Err         string
}

type DeleteTransferArgs struct {
	GID      int
	SID      int
	CID      int
	ClientId int64
	Id       int
}

type DeleteTransferReply struct {
	WrongLeader bool
	Err         string
}