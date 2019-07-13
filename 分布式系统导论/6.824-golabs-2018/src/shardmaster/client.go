package shardmaster

//
// Shardmaster clerk.
//

import "labrpc"
import "time"
import "crypto/rand"
import "math/big"

// import "fmt"

type Clerk struct {
	servers []*labrpc.ClientEnd

	clientId int64
	nextId int
	// lastSuccessfulLeaderId int
}

func nrand() int64 {
	max := big.NewInt(int64(1) << 62)
	bigx, _ := rand.Int(rand.Reader, max)
	x := bigx.Int64()
	return x
}

func MakeClerk(servers []*labrpc.ClientEnd) *Clerk {
	ck := new(Clerk)
	ck.servers = servers

	ck.clientId = nrand()
	ck.nextId = 1
	// ck.lastSuccessfulLeaderId = mrand.Intn(len(ck.servers))

	return ck
}

func (ck *Clerk) Query(num int) Config {
	args := &QueryArgs{}
	args.Num      = num
	args.ClientId = ck.clientId
	args.Id       = ck.nextId

	ck.nextId += 1

	for {
		// try each known server.
		for _, srv := range ck.servers {
			var reply QueryReply
			// fmt.Printf("%v: ShardMaster.Query\n", ck.clientId)
			ok := srv.Call("ShardMaster.Query", args, &reply)
			if ok && reply.WrongLeader == false {
				return reply.Config
			}
		}
		time.Sleep(100 * time.Millisecond)
	}
}

func (ck *Clerk) QueryOnce(num int) (Config, bool) {
	args := &QueryArgs{}
	args.Num      = num
	args.ClientId = ck.clientId
	args.Id       = ck.nextId

	ck.nextId += 1

	// try each known server.
	for _, srv := range ck.servers {
		var reply QueryReply
		ok := srv.Call("ShardMaster.Query", args, &reply)
		if ok && reply.WrongLeader == false {
			return reply.Config, true
		}
	}

	return Config{}, false
}

func (ck *Clerk) Join(servers map[int][]string) {
	args := &JoinArgs{}
	args.Servers  = servers
	args.ClientId = ck.clientId
	args.Id       = ck.nextId

	ck.nextId += 1

	for {
		// try each known server.
		for _, srv := range ck.servers {
			var reply JoinReply
			// fmt.Printf("%v: ShardMaster.Join\n", ck.clientId)
			ok := srv.Call("ShardMaster.Join", args, &reply)
			if ok && reply.WrongLeader == false {
				return
			}
		}
		time.Sleep(100 * time.Millisecond)
	}
}

func (ck *Clerk) Leave(gids []int) {
	args := &LeaveArgs{}
	args.GIDs     = gids
	args.ClientId = ck.clientId
	args.Id       = ck.nextId

	ck.nextId += 1

	for {
		// try each known server.
		for _, srv := range ck.servers {
			var reply LeaveReply
			// fmt.Printf("%v: ShardMaster.Leave\n", ck.clientId)
			ok := srv.Call("ShardMaster.Leave", args, &reply)
			if ok && reply.WrongLeader == false {
				return
			}
		}
		time.Sleep(100 * time.Millisecond)
	}
}

func (ck *Clerk) Move(shard int, gid int) {
	args := &MoveArgs{}
	args.Shard    = shard
	args.GID      = gid
	args.ClientId = ck.clientId
	args.Id       = ck.nextId

	ck.nextId += 1

	for {
		// try each known server.
		for _, srv := range ck.servers {
			var reply MoveReply
			ok := srv.Call("ShardMaster.Move", args, &reply)
			if ok && reply.WrongLeader == false {
				return
			}
		}
		time.Sleep(100 * time.Millisecond)
	}
}
