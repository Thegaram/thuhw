package substore

import "sync"

type Notification = interface{}

type SubscriptionStore struct {
    mu sync.Mutex
    subs map[int]([](chan Notification))

    // used for cleanup purposes
    numActive map[int]int
    conds map[int]*sync.Cond
}

func MakeSubStore() (*SubscriptionStore) {
    ss := new(SubscriptionStore)

    ss.subs = make(map[int]([](chan Notification)))
    ss.numActive = make(map[int]int)
    ss.conds = make(map[int]*sync.Cond)

    return ss
}

func (ss *SubscriptionStore) Subscribe(index int) ((chan Notification)) {
    ss.mu.Lock()
    defer ss.mu.Unlock()

    _, ok := ss.subs[index]

    // allocate new channel slice and make sure it's cleaned up eventually
    if !ok {
        ss.subs[index] = make([](chan Notification), 0)
        ss.conds[index] = sync.NewCond(&ss.mu)

        go func(index int, cond *sync.Cond){
            cond.L.Lock()
            defer cond.L.Unlock()

            for ss.numActive[index] > 0 {
                cond.Wait() // release lock until next Signal
            }

            delete(ss.subs, index)
            delete(ss.numActive, index)
            delete(ss.conds, index)
        }(index, ss.conds[index])
    }

    sub := make((chan Notification), 1) // at most one result per index!
    ss.subs[index] = append(ss.subs[index], sub)
    ss.numActive[index] += 1

    return sub
}

func (ss *SubscriptionStore) Release(index int) {
    ss.mu.Lock()
    defer ss.mu.Unlock()

    ss.numActive[index] -= 1
    ss.conds[index].Signal()
}

func (ss *SubscriptionStore) GetSubs(index int) ([](chan Notification), bool) {
    ss.mu.Lock()
    defer ss.mu.Unlock()

    subs, ok := ss.subs[index]
    return subs, ok
}

func (ss *SubscriptionStore) NumEntries() (int) {
    ss.mu.Lock()
    defer ss.mu.Unlock()
    return len(ss.subs)
}

func (ss *SubscriptionStore) NotifyAll(index int, not interface{}) {
    if subs, ok := ss.GetSubs(index); ok {
        for _, s := range subs {
            s <- not // buffered; should not block
        }
    }
}