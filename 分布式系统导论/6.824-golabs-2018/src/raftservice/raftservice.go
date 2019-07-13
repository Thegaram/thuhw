package raftservice

import "raft"
import "substore"
import "sync"
import "time"
import "util"

const CONSENSUS_TIMEOUT = 5 * 100 * time.Millisecond

type Notification struct{
    Id string
    Res interface{}
}

const (
    OK = "OK"
    WrongLeaderErr = "Wrong Leader"
    ConcurrentErr  = "Replaced by concurrent request"
    TimeoutErr     = "Timeout"
)

type Err string
type applyFunc func(raft.ApplyMsg) (string, interface{})
type controlFunc func(raft.ApplyMsg)

type RaftService struct {
    mu sync.Mutex

    rf *raft.Raft
    applyCh chan raft.ApplyMsg

    apply applyFunc
    control controlFunc

    subs *substore.SubscriptionStore

    active bool
}

func StartService(rf *raft.Raft, apply applyFunc, control controlFunc, applyCh chan raft.ApplyMsg) (*RaftService) {
    rs := new(RaftService)

    rs.rf = rf
    rs.applyCh = applyCh

    rs.apply = apply
    rs.control = control

    rs.subs = substore.MakeSubStore()
    rs.active = true

    go rs.applierLoop()

    return rs
}

func (rs *RaftService) Kill() {
    rs.mu.Lock()
    defer rs.mu.Unlock()
    rs.active = false
}

func (rs *RaftService) isActive() bool {
    rs.mu.Lock()
    defer rs.mu.Unlock()
    return rs.active
}

func (rs *RaftService) Start(id string, op interface{}) (string, interface{}) {
    index, _, isLeader := rs.rf.Start(op)
    if !isLeader { return WrongLeaderErr, nil }

    sub := rs.subs.Subscribe(index)
    defer rs.subs.Release(index)

    select {
    case raw_notification := <-sub:
        notification, ok := raw_notification.(Notification)
        util.Assert(ok)

        if notification.Id != id {
            return ConcurrentErr, nil
        }

        return OK, notification.Res

    case <-time.After(CONSENSUS_TIMEOUT):
        return TimeoutErr, nil
    }
}

func (rs *RaftService) applierLoop() {
    for msg := range rs.applyCh {
        if !rs.isActive() { return }

        if (msg.CommandValid) {
            id, res := rs.apply(msg)
            rs.subs.NotifyAll(msg.CommandIndex, Notification{id, res})
        } else {
            rs.control(msg)
        }
    }
}