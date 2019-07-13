package raftkv

import "labrpc"
import "crypto/rand"
import "math/big"
import mrand "math/rand"


type Clerk struct {
	servers []*labrpc.ClientEnd

	clientId int64
	nextId int
	lastSuccessfulLeaderId int
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
	ck.lastSuccessfulLeaderId = mrand.Intn(len(ck.servers))

	return ck
}

func (ck *Clerk) Get(key string) string {
	s := ck.lastSuccessfulLeaderId
	args := GetArgs{key}

	for {
		reply := GetReply{}
		ok := ck.servers[s].Call("KVServer.GetRPC", &args, &reply)

		if !ok || reply.WrongLeader {
			s = mrand.Intn(len(ck.servers))
			continue // retry
		}

		if reply.Err == ErrNoKey {
			return ""
		}

		if reply.Err != OK {
			s = mrand.Intn(len(ck.servers))
			continue // retry
		}

		ck.lastSuccessfulLeaderId = s
		return reply.Value
	}
}

func (ck *Clerk) PutAppend(key string, value string, op string) {
	s := ck.lastSuccessfulLeaderId
	args := PutAppendArgs{key, value, op, ck.clientId, ck.nextId}
	ck.nextId += 1

	for {
		reply := PutAppendReply{}
		ok := ck.servers[s].Call("KVServer.PutAppendRPC", &args, &reply)

		if !ok || reply.WrongLeader || reply.Err != OK {
			s = mrand.Intn(len(ck.servers))
			continue // retry
		}

		ck.lastSuccessfulLeaderId = s
		return
	}
}

func (ck *Clerk) Put(key string, value string) {
	ck.PutAppend(key, value, "Put")
}

func (ck *Clerk) Append(key string, value string) {
	ck.PutAppend(key, value, "Append")
}
