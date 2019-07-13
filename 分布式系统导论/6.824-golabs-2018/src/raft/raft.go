package raft

import "bytes"
import "log"
import "math/rand"
import "sync"
import "time"

import "labgob"
import "labrpc"


///////////////////////////////////////////////////////////////////
/////////////////////// CONSTANTS & HELPERS ///////////////////////
///////////////////////////////////////////////////////////////////

const SLEEP_PERIOD 		 =  10 * time.Millisecond
const HEARTBEAT_INTERVAL = 110 * time.Millisecond

const ELECTION_TIMEOUT_MIN = 400 // ms
const ELECTION_TIMEOUT_MAX = 800 // ms

const NONEXISTENT_TERM = -1000

func getRandomElectionTimeout() time.Duration {
	ms := rand.Intn(ELECTION_TIMEOUT_MAX - ELECTION_TIMEOUT_MIN) + ELECTION_TIMEOUT_MIN
	return time.Duration(ms) * time.Millisecond
}


///////////////////////////////////////////////////////////////////
//////////////////////// TYPE DECLARATIONS ////////////////////////
///////////////////////////////////////////////////////////////////

type ApplyMsg struct {
	CommandValid bool
	Command      interface{}
	CommandIndex int
}

type Log struct {
	Command interface{}
	Term 	int
}

type state_t int

const (
	Follower state_t = iota
	Candidate
	Leader
)

func (s state_t) toString() string {
	switch s {
		case Follower : return " Follower"
		case Candidate: return "Candidate"
		case Leader   : return "   Leader"
	}

	assert(false, "encountered unknown state ", s)
	return "Unknown" // unreachable
}


///////////////////////////////////////////////////////////////////
//////////////////////// STATE & MODIFIERS ////////////////////////
///////////////////////////////////////////////////////////////////

// a Go object implementing a single Raft peer
type Raft struct {
	mu        sync.Mutex          // lock to protect shared access to this peer's state
	peers     []*labrpc.ClientEnd // RPC endpoints of all peers
	persister *Persister          // object to hold this peer's persisted state
	me        int                 // this peer's index into peers[]

	// instance state below

	state state_t
	currentTerm int

	log []Log
	snapshotLastIndex int

	commitIndex int // index of last committed msg
	lastApplied int // index of last applied msg (<= commitIndex)

	lastHeardFromLeader time.Time // last msg received from leader (followers only)
	lastHeartbeat time.Time // last heartbeat sent to followers (leaders only)

	numVotes int

	nextIndex []int
	matchIndex []int

	active bool

	applyCh chan ApplyMsg
}

func (rf *Raft) forAllPeers(fn func(int)) {
	for peer := 0; peer < len(rf.peers); peer += 1 {
		if peer == rf.me { continue }
		fn(peer)
	}
}

func (rf *Raft) isActive() bool {
	rf.mu.Lock()
	defer rf.mu.Unlock()
	return rf.active
}

// assume rf is locked by caller...
func (rf *Raft) DPrintf(format string, a ...interface{}) {
	if !rf.active { return }

	args := make([]interface{}, 0)
	args = append(args, rf.state.toString())
	args = append(args, rf.me)
	args = append(args, rf.currentTerm)
	args = append(args, rf.snapshotLastIndex)
	args = append(args, rf.logLen())
	args = append(args, rf.lastApplied)
	args = append(args, rf.commitIndex)
	args = append(args, a...)

	DPrintf("RAFT [%s(%d)@%2d, logs: [%d...%d), A=%d, C=%d] " + format, args...)
}

// assume rf is locked by caller...
func (rf *Raft) logLen() (int) {
	return rf.snapshotLastIndex + len(rf.log)
}

// assume rf is locked by caller...
func (rf *Raft) logEntry(index int) (Log) {
	newIndex := index - rf.snapshotLastIndex

	assert(newIndex >= 0)
	assert(newIndex < len(rf.log))

	return rf.log[newIndex]
}

// assume rf is locked by caller...
func (rf *Raft) copyLogEntriesInclusive(from int, to int) []Log {
	entries := make([]Log, to - from + 1)

	fromIndex := from - rf.snapshotLastIndex
	toIndex   := to   - rf.snapshotLastIndex

	copy(entries, rf.log[fromIndex : toIndex + 1])

	return entries
}

// assume rf is locked by caller...
// rf.commitIndex and rf.lastApplied must be updated prior to this call
// rf.snapshotLastIndex will be updated during this call
func (rf *Raft) trimLogHead(lastIncludedIndex int) {
	assert(lastIncludedIndex < rf.logLen())
	assert(lastIncludedIndex >= rf.snapshotLastIndex)
	assert(rf.commitIndex >= lastIncludedIndex)
	assert(rf.lastApplied >= lastIncludedIndex)
	rf.DPrintf("trimming log up to %d", lastIncludedIndex)

	newLog := make([]Log, 1)
	newLog[0] = Log { "snapshot", rf.logEntry(lastIncludedIndex).Term }
	newLog = append(newLog, rf.copyLogEntriesInclusive(lastIncludedIndex + 1, rf.logLen() - 1)...)

	rf.log = newLog
	rf.snapshotLastIndex = lastIncludedIndex

	// NOTE: do not persist here
	// that way the persisted state/snapshot pair will be transiently inconsistent
}

// assume rf is locked by caller...
func (rf *Raft) trimLogTail(firstIndexToDrop int) {
	assert(firstIndexToDrop <= rf.logLen())
	assert(rf.commitIndex <= firstIndexToDrop)
	rf.DPrintf("trimming log from %d", firstIndexToDrop)

	index := firstIndexToDrop - rf.snapshotLastIndex
	rf.log = rf.log[0:index]

	// NOTE: do not persist here
}

// assume rf is locked by caller...
func (rf *Raft) becomeFollower(newTerm int) {
	assert(newTerm >= rf.currentTerm)

	oldTerm := rf.currentTerm

	rf.state = Follower
	rf.currentTerm = newTerm

	if oldTerm != newTerm {
		rf.persist()
	}
}

// assume rf is locked by caller...
func (rf *Raft) acknowledgeLeader(newTerm int) {
	rf.becomeFollower(newTerm)
	rf.lastHeardFromLeader = time.Now() // not always accurate (e.g. on RPC replies) but good enough!
}

// assume rf is locked by caller...
func (rf *Raft) becomeCandidate() {
	assert(rf.state != Leader)

	rf.state = Candidate
	rf.currentTerm += 1
	rf.numVotes = 1
	rf.lastHeardFromLeader = time.Now()

	rf.persist()
}

// assume rf is locked by caller...
func (rf *Raft) appendLogs(newLogs ...Log) {
	rf.log = append(rf.log, newLogs...)
	rf.persist()
}

// assume rf is locked by caller...
func (rf *Raft) majorityGranted() {
	assert(rf.state == Candidate)
	rf.DPrintf("majorityGranted (#peers = %d)", rf.numVotes)

	// initialize state
	rf.state 	= Leader
	rf.numVotes = 0

	// initialize peer metadata
	rf.forAllPeers(func (peer int) {
		rf.nextIndex[peer]  = rf.logLen()
		rf.matchIndex[peer] = 0
	})

	rf.persist()
}

// assume rf is locked by caller...
func (rf *Raft) receiveVote() {
	rf.numVotes += 1

	if rf.numVotes > (len(rf.peers) / 2) {
		rf.majorityGranted()
		// note: heartbeat is automatically sent by a dedicated worker loop
	}
}

// assume rf is locked by caller...
func (rf *Raft) setCommitIndex(index int) {
	if index < rf.snapshotLastIndex { return } // TODO: is this possible?

	assert(index > rf.commitIndex) // monotonically increasing
	assert(index < rf.logLen())    // cannot over-index

	if rf.state == Leader && rf.logEntry(index).Term != rf.currentTerm {
		rf.DPrintf("not updating commit index to %d, term not matching", index)
		return
	}

	originalCommitIndex := rf.commitIndex

	rf.DPrintf("updating commit index to %d", index)
	rf.commitIndex = index

	// try to apply newly committed logs
	if rf.commitIndex > originalCommitIndex {
		go rf.tryApply()
	}
}

// assume rf is locked by caller...
func (rf *Raft) setMatchIndex(peer int, index int) {
	if index < rf.matchIndex[peer] { return }
	assert(index >= rf.matchIndex[peer]) // monotonically increasing
	assert(index < rf.logLen())			 // cannot over-index
	assert(rf.state == Leader)
	// assert(rf.logEntry(index).Term == rf.currentTerm) // TODO: review this

	rf.DPrintf("updating %d's match index to %d", peer, index)

	rf.matchIndex[peer] = index

	// update commit index
	for ii := index; ii > rf.commitIndex; ii-- {
		count := 1 // count "me" (most up-to-date)

		rf.forAllPeers(func (p int) {
			if rf.matchIndex[p] >= ii {
				count++
			}
		})

		if count > (len(rf.peers) / 2) {
			rf.setCommitIndex(ii)
			break
		}
	}
}

// assume rf is locked by caller...
func (rf *Raft) serialize() []byte {
	w := new(bytes.Buffer)
	e := labgob.NewEncoder(w)

	e.Encode(rf.currentTerm)
	e.Encode(rf.snapshotLastIndex)
	e.Encode(rf.log)

	return w.Bytes()
}

// assume rf is locked by caller...
func (rf *Raft) persist() {
	state := rf.serialize()
	rf.persister.SaveRaftState(state)
}

func (rf *Raft) readPersist(data []byte) {
	if data == nil || len(data) < 1 { return }
	rf.DPrintf("reading persisted state")

	r := bytes.NewBuffer(data)
	d := labgob.NewDecoder(r)

	var currentTerm int
	var snapshotLastIndex int
	var logs []Log

	if err := d.Decode(&currentTerm); err != nil {
		log.Fatal("Unable to deserialize currentTerm", err)
	}

	if err := d.Decode(&snapshotLastIndex); err != nil {
		log.Fatal("Unable to deserialize snapshotLastIndex", err)
	}

	if err := d.Decode(&logs); err != nil {
		log.Fatal("Unable to deserialize log", err)
	}

	rf.currentTerm = currentTerm
	rf.snapshotLastIndex = snapshotLastIndex
	rf.log = logs

	// avoid erroneously re-applying logs
	rf.commitIndex = rf.snapshotLastIndex
	rf.lastApplied = rf.snapshotLastIndex

	rf.DPrintf("read persisted state")
	rf.DPrintf("%+v", rf.log)
	rf.DPrintf("%d: %+v", rf.persister.RaftStateSize(), data)
}


///////////////////////////////////////////////////////////////////
//////////////////////// REQUEST VOTE RPC /////////////////////////
///////////////////////////////////////////////////////////////////

type RequestVoteArgs struct {
	Term 		 int
	CandidateId  int
	LastLogIndex int
	LastLogTerm  int
}

type RequestVoteReply struct {
	Term 		int
	VoteGranted bool
}

func (rf *Raft) sendRequestVoteRPC(server int, args *RequestVoteArgs, reply *RequestVoteReply) bool {
	ok := rf.peers[server].Call("Raft.RequestVoteRPCHandler", args, reply)
	return ok
}

func (rf *Raft) requestVote(peer int, term int) {
	// ----------------------------------------------------
	rf.mu.Lock()

	if !rf.active || rf.state != Candidate || rf.currentTerm != term {
		rf.mu.Unlock()
		return
	}

	args := RequestVoteArgs {}
	args.Term         = rf.currentTerm
	args.CandidateId  = rf.me
	args.LastLogIndex = rf.logLen() - 1
	args.LastLogTerm  = rf.logEntry(args.LastLogIndex).Term

	rf.DPrintf("sending RequestVote %+v", args)
	rf.mu.Unlock()
	// ----------------------------------------------------

	// NOTE: do not lock around RPC call
	reply := RequestVoteReply {}
	success := rf.sendRequestVoteRPC(peer, &args, &reply)
	if !success { return }
	// TODO: retry request on failure
	// NOTE: retry logic is tricky; peer should be able to grant vote multiple times

	// ----------------------------------------------------
	rf.mu.Lock()
	defer rf.mu.Unlock()
	rf.DPrintf("received RequestVote reply: %+v", reply)

	if (reply.Term > rf.currentTerm) {
		rf.acknowledgeLeader(reply.Term)
		return
	}

	if !rf.active || !reply.VoteGranted || rf.state != Candidate || rf.currentTerm != term {
		return
	}

	rf.receiveVote()
	// ----------------------------------------------------
}

func (rf *Raft) RequestVoteRPCHandler(args *RequestVoteArgs, reply *RequestVoteReply) {
	rf.mu.Lock()
	defer rf.mu.Unlock()
	if !rf.active { return }
	rf.DPrintf("incoming RequestVote: %+v", args)

	if args.Term <= rf.currentTerm {
		rf.DPrintf("rejecting RequestVote: outdated/concurrent")
		reply.Term = rf.currentTerm
		reply.VoteGranted = false
		return
	}

	// regardless of our vote, there is a higher term, so we step down
	rf.becomeFollower(args.Term)

	// check if log up-to-date
	lastLogIndex := rf.logLen() - 1
	lastLogTerm  := rf.logEntry(lastLogIndex).Term

	if lastLogTerm > args.LastLogTerm || (lastLogTerm == args.LastLogTerm && lastLogIndex > args.LastLogIndex) {
		rf.DPrintf("rejecting RequestVote: log not up-to-date %+v, lastLogIndex:%d, lastLogTerm:%d", args, lastLogIndex, lastLogTerm)
		reply.Term = rf.currentTerm
		reply.VoteGranted = false
		return
	}

	// grant vote
	rf.DPrintf("vote granted %+v, lastLogIndex:%d, lastLogTerm:%d", args, lastLogIndex, lastLogTerm)

	reply.Term = args.Term
	reply.VoteGranted = true

	rf.acknowledgeLeader(args.Term)
}


///////////////////////////////////////////////////////////////////
//////////////////////// APPEND ENTRIES RPC ///////////////////////
///////////////////////////////////////////////////////////////////

type AppendEntriesArgs struct {
	Term 		 int
	LeaderId	 int
	PrevLogIndex int
	PrevLogTerm  int
	Entries 	 []Log
	LeaderCommit int
}

type AppendEntriesReply struct {
	Term 		  int
	Success 	  bool
	ConflictTerm  int
	ConflictIndex int
}

func (rf *Raft) sendAppendEntriesRPC(server int, args *AppendEntriesArgs, reply *AppendEntriesReply) bool {
	ok := rf.peers[server].Call("Raft.AppendEntriesRPCHandler", args, reply)
	return ok
}

func (rf *Raft) startAppendLoop(peer int, term int, index int) {
	for {
		// ----------------------------------------------------
		rf.mu.Lock()

		if !rf.active || rf.state != Leader || rf.currentTerm != term {
			rf.mu.Unlock()
			return
		}

		// non-existent log or new loop has been started
		if index >= rf.logLen() || /*rf.nextIndex[peer] > index ||*/ rf.logLen() > index + 1 {
			rf.mu.Unlock()
			return
		}

		// peer lagging behind: send snapshot first
		if rf.nextIndex[peer] <= rf.snapshotLastIndex {
			rf.mu.Unlock()
			rf.sendInstallSnapshot(peer, term) // block until succeed
			continue
		}

		args := AppendEntriesArgs{}
		args.Term         = rf.currentTerm
		args.LeaderId     = rf.me
		args.PrevLogIndex = rf.nextIndex[peer] - 1
		args.PrevLogTerm  = rf.logEntry(args.PrevLogIndex).Term
		args.Entries      = rf.copyLogEntriesInclusive(rf.nextIndex[peer], index)
		args.LeaderCommit = rf.commitIndex

		rf.DPrintf("sending AppendEntries to peer %d: %+v", peer, args)
		rf.mu.Unlock()
		// ----------------------------------------------------

		// NOTE: do not lock around RPC call
		reply := AppendEntriesReply{}
		success := rf.sendAppendEntriesRPC(peer, &args, &reply)
		if !success { continue } // try again

		// ----------------------------------------------------
		// NOTE: cannot use `defer Unlock()` due to loop
		rf.mu.Lock()
		rf.DPrintf("received AppendEntries reply from peer %d: %+v", peer, reply)

		if reply.Term > rf.currentTerm {
			rf.acknowledgeLeader(reply.Term)
			rf.mu.Unlock()
			return
		}

		if !rf.active || rf.state != Leader || rf.currentTerm != term {
			rf.mu.Unlock()
			return
		}

		if reply.Success {
			rf.nextIndex[peer] = index + 1
			rf.setMatchIndex(peer, index)
			rf.mu.Unlock()
			return
		}

		// ----------------------------------------------------
		// accelerated log backtracking optimization
        if reply.ConflictIndex <= rf.snapshotLastIndex {
			rf.nextIndex[peer] = rf.snapshotLastIndex // will send snapshot in next round
			rf.mu.Unlock()
			continue
		}

        rf.nextIndex[peer] = reply.ConflictIndex // prefer simplicity over fully optimal code...
		rf.mu.Unlock()
		// ----------------------------------------------------
	}
}

func (rf *Raft) AppendEntriesRPCHandler(args *AppendEntriesArgs, reply *AppendEntriesReply) {
	// ----------------------------------------------------
	rf.mu.Lock()
	defer rf.mu.Unlock()
	if !rf.active { return }
	rf.DPrintf("incoming AppendEntries: %+v", args)

	if args.Term < rf.currentTerm {
		rf.DPrintf("rejecting AppendEntries: outdated")
		reply.Term = rf.currentTerm
		reply.Success = false
		return
	}

	rf.acknowledgeLeader(args.Term)

	// ----------------------------------------------------
	// perform consistency check a)
	if args.PrevLogIndex >= rf.logLen() {
		reply.Term          = rf.currentTerm
		reply.Success       = false
		reply.ConflictIndex = rf.logLen()
		reply.ConflictTerm  = NONEXISTENT_TERM

		rf.DPrintf("rejecting AppendEntries: failed consistency check a)")
		return
	}

	// perform consistency check b)
	// ideally, this should not happen: this would mean
	// the leader's syncing committed logs we already have
	// TODO: review this
	if args.PrevLogIndex < rf.snapshotLastIndex {
		reply.Term          = rf.currentTerm
		reply.Success       = false
		reply.ConflictIndex = rf.snapshotLastIndex + 1 // start from subsequent (potentially uncommitted) logs
		reply.ConflictTerm  = NONEXISTENT_TERM

		rf.DPrintf("rejecting AppendEntries: failed consistency check b)")
		return
	}

	// perform consistency check c)
	if rf.logEntry(args.PrevLogIndex).Term != args.PrevLogTerm {
		reply.Term          = rf.currentTerm
		reply.Success       = false
		reply.ConflictIndex = args.PrevLogIndex
		reply.ConflictTerm  = rf.logEntry(args.PrevLogIndex).Term

		// accelerated log backtracking optimization
		for ii := args.PrevLogIndex - 1; ii >= rf.snapshotLastIndex; ii-- {
			if rf.logEntry(ii).Term != reply.ConflictTerm { break }
			reply.ConflictIndex = ii
		}
		// corner case: reply.ConflictIndex == rf.snapshotLastIndex
		// TODO: think this through...
		// this is not so good as it will trigger b) on the next round

		rf.DPrintf("rejecting AppendEntries: failed consistency check c)")
		return
	}
	// ----------------------------------------------------

	// append entries
	for ii, entry := range args.Entries {
		index := args.PrevLogIndex + ii + 1

		// if there is a conflict: trim log
		if index < rf.logLen() && rf.logEntry(index).Term != entry.Term {
			rf.trimLogTail(index)
		}

		if index >= rf.logLen() {
			rf.appendLogs(args.Entries[ii:]...)
			break
		}

		assert(index < rf.logLen())
		assert(rf.logEntry(index).Term == entry.Term)
	}

	// update commit index
	if args.LeaderCommit > rf.commitIndex {
		rf.setCommitIndex(min(args.LeaderCommit, rf.logLen() - 1))
	}

	reply.Term = rf.currentTerm
	reply.Success = true
	// ----------------------------------------------------
}


///////////////////////////////////////////////////////////////////
///////////////////// INSTALL SNAPSHOT RPC ////////////////////////
///////////////////////////////////////////////////////////////////

type InstallSnapshotArgs struct {
	Term              int
	LeaderId          int
	LastIncludedIndex int
	LastIncludedTerm  int
	Data              []byte
}

type InstallSnapshotReply struct {
	Success bool
	Term    int
}

func (rf *Raft) sendInstallSnapshotRPC(server int, args *InstallSnapshotArgs, reply *InstallSnapshotReply) bool {
	ok := rf.peers[server].Call("Raft.InstallSnapshotRPCHandler", args, reply)
	return ok
}

func (rf *Raft) sendInstallSnapshot(peer int, term int) {
	// ----------------------------------------------------
	rf.mu.Lock()

	if !rf.active || rf.state != Leader || rf.currentTerm != term {
		rf.mu.Unlock()
		return
	}

	args := InstallSnapshotArgs {}
	args.Term              = rf.currentTerm
	args.LeaderId          = rf.me
	args.LastIncludedIndex = rf.snapshotLastIndex
	args.LastIncludedTerm  = rf.logEntry(rf.snapshotLastIndex).Term
	args.Data              = rf.persister.ReadSnapshot()

	rf.DPrintf("sending InstallSnapshot: %+v", args)
	rf.mu.Unlock()
	// ----------------------------------------------------

	// NOTE: do not lock around RPC call
	reply := InstallSnapshotReply{}
	success := rf.sendInstallSnapshotRPC(peer, &args, &reply)
	if !success { return }
	// NOTE: no need to retry here; we'll retry in startAppendLoop

	// ----------------------------------------------------
	rf.mu.Lock()
	defer rf.mu.Unlock()
	rf.DPrintf("received InstallSnapshot reply: %+v", reply)

	if reply.Term > rf.currentTerm {
		rf.acknowledgeLeader(reply.Term)
		return
	}

	if !rf.active || rf.state != Leader || rf.currentTerm != term || !reply.Success {
		return // NOTE: we'll retry in startAppendLoop
	}

	rf.nextIndex[peer]  = rf.snapshotLastIndex + 1
	rf.setMatchIndex(peer, rf.snapshotLastIndex)
	// ----------------------------------------------------
}

// simple application-specific assert
// only for debugging; Raft should not know about snapshot format!
func assertSnapshotConsistent(data []byte, expectedIndex int) {
	// copy first 8 bytes (int might be 32b-64b)
	// snapshot := make([]byte, 8)
	// copy(snapshot, data[0:8])

	// r := bytes.NewBuffer(snapshot)
	// d := labgob.NewDecoder(r)

	// var lastIncludedIndex int
	// if err := d.Decode(&lastIncludedIndex); err != nil {
	// 	log.Fatal("Unable to deserialize lastIncludedIndex", err)
	// }

	// assert(expectedIndex == lastIncludedIndex, "lastIncludedIndex = %d, expected: %d", lastIncludedIndex, expectedIndex)
}

func (rf *Raft) Snapshot(lastIncludedIndex int, snapshot []byte) {
	rf.mu.Lock()
	defer rf.mu.Unlock()

	// someone truncated the log in the meantime => terminate (try again later)
	// this can happen through a) concurrent Snapshot call, b) incoming InstallSnapshot RPC
	if (lastIncludedIndex < rf.snapshotLastIndex) {
		return
	}

	assertSnapshotConsistent(snapshot, lastIncludedIndex)

	assert(rf.commitIndex >= lastIncludedIndex)
	assert(rf.lastApplied >= lastIncludedIndex)
	// NOTE: might be greater if we applied one more since the last call

	rf.trimLogHead(lastIncludedIndex)
	state := rf.serialize()
	rf.persister.SaveStateAndSnapshot(state, snapshot)

	rf.DPrintf("saved snapshot: %v $v", state, snapshot)
}

func (rf *Raft) InstallSnapshotRPCHandler(args *InstallSnapshotArgs, reply *InstallSnapshotReply) {
	// ----------------------------------------------------
	rf.mu.Lock()
	defer rf.mu.Unlock() // block everything else (e.g. applierLoop) until we apply snapshot...
	if !rf.active { return }
	rf.DPrintf("received InstallSnapshot RPC: %+v", args)
	assertSnapshotConsistent(args.Data, args.LastIncludedIndex)

	if args.Term < rf.currentTerm {
		reply.Term    = rf.currentTerm
		reply.Success = false
		return
	}

	rf.acknowledgeLeader(args.Term)

	// helper function (assume rf is locked by the caller...)
	persistAndApplySnapshot := func(snapshot []byte) {
		// persist
		state := rf.serialize()
		rf.persister.SaveStateAndSnapshot(state, snapshot)

		// apply
		rf.applyCh <- ApplyMsg{ false, snapshot, -1 }
		rf.DPrintf("applied snapshot")
	}
	// ----------------------------------------------------

	// outdated snapshot => reject
	// old log: *..............|-------A------------C----------------|
	// new log:                |-------A------------C----------------|
	if args.LastIncludedIndex < rf.snapshotLastIndex {
		rf.DPrintf("rejecting InstallSnapshot: outdated (%d < %d)", args.LastIncludedIndex, rf.snapshotLastIndex)
		reply.Term    = rf.currentTerm
		reply.Success = false
		return
	}

	// follower lagging behind
	// old log: |----------A--------------C-----------|.............*
	// new log:                                                    |*AC|
	if args.LastIncludedIndex >= rf.logLen() {
		rf.DPrintf("accepting InstallSnapshot: a) (%d >= %d)", args.LastIncludedIndex, rf.logLen())

		// update indices
		rf.commitIndex = max(rf.commitIndex, args.LastIncludedIndex) // should be args.LastIncludedIndex
		rf.lastApplied = max(rf.lastApplied, args.LastIncludedIndex) // should be args.LastIncludedIndex

		// save snapshot as local log
		rf.log = make([]Log, 1)
		rf.log[0] = Log{ "snapshot", args.LastIncludedTerm }
		rf.snapshotLastIndex = args.LastIncludedIndex

		persistAndApplySnapshot(args.Data)

		reply.Term    = rf.currentTerm
		reply.Success = true
		return
	}

	// local log conflicts with snapshot
	// old log: |----A-----C------*!-------------------|
	// new log:                  |*AC|
	if rf.logEntry(args.LastIncludedIndex).Term != args.LastIncludedTerm {
		rf.DPrintf("accepting InstallSnapshot: b) (%d != %d)", rf.logEntry(args.LastIncludedIndex).Term, args.LastIncludedTerm)

		// if we need to throw away committed logs, something's wrong...
		assert(rf.commitIndex < args.LastIncludedIndex)
		assert(rf.lastApplied < args.LastIncludedIndex)

		// update indices
		rf.commitIndex = max(rf.commitIndex, args.LastIncludedIndex) // should be args.LastIncludedIndex
		rf.lastApplied = max(rf.lastApplied, args.LastIncludedIndex) // should be args.LastIncludedIndex

		// save snapshot as local log
		rf.log = make([]Log, 1)
		rf.log[0] = Log{ "snapshot", args.LastIncludedTerm }
		rf.snapshotLastIndex = args.LastIncludedIndex

		persistAndApplySnapshot(args.Data)

		reply.Term    = rf.currentTerm
		reply.Success = true
		return
	}


	// snapshot contains uncommitted logs
	// old log: |------A-----C----*-------------------|
	// new log:                  |*AC-----------------|
	if rf.commitIndex < args.LastIncludedIndex {
		rf.DPrintf("accepting InstallSnapshot: c) (%d < %d)", rf.commitIndex, args.LastIncludedIndex)

		// sanity check
		assert(rf.lastApplied <= rf.commitIndex)

		// update indices
		rf.commitIndex = max(rf.commitIndex, args.LastIncludedIndex) // should be args.LastIncludedIndex
		rf.lastApplied = max(rf.lastApplied, args.LastIncludedIndex) // should be args.LastIncludedIndex

		// trim log
		rf.trimLogHead(args.LastIncludedIndex)
		// NOTE: rf.snapshotLastIndex updated by rf.trimLogHead

		persistAndApplySnapshot(args.Data)

		reply.Term    = rf.currentTerm
		reply.Success = true
		return
	}

	// snapshot contains unapplied logs
	// old log: |------A----------*----------C--------|
	// new log:                  |*A---------C--------|
	if rf.lastApplied < args.LastIncludedIndex {
		rf.DPrintf("accepting InstallSnapshot: d) (%d < %d)", rf.lastApplied, args.LastIncludedIndex)

		// sanity check
		assert(rf.commitIndex >= args.LastIncludedIndex)

		// update indices
		rf.commitIndex = max(rf.commitIndex, args.LastIncludedIndex) // should be rf.commitIndex
		rf.lastApplied = max(rf.lastApplied, args.LastIncludedIndex) // should be args.LastIncludedIndex

		// trim log
		rf.trimLogHead(args.LastIncludedIndex)
		// NOTE: rf.snapshotLastIndex updated by rf.trimLogHead

		persistAndApplySnapshot(args.Data)

		reply.Term    = rf.currentTerm
		reply.Success = true
		return
	}

	// snapshot is just an update
	// old log: |-----------------*----A-----C--------|
	// new log:                  |*----A-----C--------|
	if rf.lastApplied >= args.LastIncludedIndex {
		rf.DPrintf("accepting InstallSnapshot: e) (%d >= %d)", rf.lastApplied, args.LastIncludedIndex)

		// sanity check
		assert(rf.lastApplied <= rf.commitIndex)

		// update indices
		rf.commitIndex = max(rf.commitIndex, args.LastIncludedIndex) // should be rf.commitIndex
		rf.lastApplied = max(rf.lastApplied, args.LastIncludedIndex) // should be rf.lastApplied

		// trim log
		rf.trimLogHead(args.LastIncludedIndex)
		// NOTE: rf.snapshotLastIndex updated by rf.trimLogHead

		// persist
		state := rf.serialize()
		rf.persister.SaveStateAndSnapshot(state, args.Data)

		// NOTE: no need to apply, we've applied all changes previously

		reply.Term    = rf.currentTerm
		reply.Success = true
		return
	}

	assert(false) // should not reach this point
	// ----------------------------------------------------
}


///////////////////////////////////////////////////////////////////
////////////////////////// WORKER LOOPS ///////////////////////////
///////////////////////////////////////////////////////////////////

func (rf *Raft) electionLoop() {
	electionTimeout := getRandomElectionTimeout()

	for {
		rf.mu.Lock()

		if !rf.active {
			rf.mu.Unlock()
			return
		}

		if rf.state != Leader && time.Since(rf.lastHeardFromLeader) >= electionTimeout {
			rf.DPrintf("triggering election")
			rf.becomeCandidate()
			rf.forAllPeers(func (peer int) { go rf.requestVote(peer, rf.currentTerm) })
			electionTimeout = getRandomElectionTimeout()
		}

		rf.mu.Unlock()
		time.Sleep(SLEEP_PERIOD)
	}
}

func (rf *Raft) heartbeatLoop() {
	for {
		rf.mu.Lock()

		if !rf.active {
			rf.mu.Unlock()
			return
		}

		if rf.state == Leader && time.Since(rf.lastHeartbeat) >= HEARTBEAT_INTERVAL {
			rf.DPrintf("sending out heartbeats")
			rf.forAllPeers(func (peer int) { go rf.startAppendLoop(peer, rf.currentTerm, rf.logLen() - 1 )})
			rf.lastHeartbeat = time.Now()
		}

		rf.mu.Unlock()
		time.Sleep(SLEEP_PERIOD)
	}
}

func (rf *Raft) tryApply() {
	rf.mu.Lock()
	defer rf.mu.Unlock()

	assert(rf.lastApplied >= rf.snapshotLastIndex)
	assert(rf.lastApplied <= rf.commitIndex)

	if !rf.active {
		return
	}

	// try to append as many logs as we can
	for ii := rf.lastApplied + 1; ii <= rf.commitIndex; ii++ {
		msg := ApplyMsg {}
		msg.CommandValid = true
		msg.Command      = rf.logEntry(ii).Command
		msg.CommandIndex = ii

		select {
		case rf.applyCh <- msg: // assume large-enough buffer...
			rf.DPrintf("applied log #%d: %+v", ii, msg)

		// default: // try again later
		case <-time.After(1 * time.Millisecond):
			rf.DPrintf("failed to apply log #%d", ii)
			return
		}
		// rf.applyCh <- msg

		rf.lastApplied = ii
	}
}

func (rf *Raft) applierLoop() {
	for {
		if !rf.isActive() {
			close(rf.applyCh)
			return
		}

		rf.tryApply()
		time.Sleep(SLEEP_PERIOD)
	}
}


///////////////////////////////////////////////////////////////////
/////////////////////////// INTERFACE /////////////////////////////
///////////////////////////////////////////////////////////////////

func (rf *Raft) GetState() (int, bool) {
	rf.mu.Lock()
	defer rf.mu.Unlock()
	return rf.currentTerm, (rf.state == Leader)
}

func (rf *Raft) Kill() {
	rf.mu.Lock()
	defer rf.mu.Unlock()
	rf.active = false
}

//
// the service using Raft (e.g. a k/v server) wants to start
// agreement on the next command to be appended to Raft's log. if this
// server isn't the leader, returns false. otherwise start the
// agreement and return immediately. there is no guarantee that this
// command will ever be committed to the Raft log, since the leader
// may fail or lose an election. even if the Raft instance has been killed,
// this function should return gracefully.
//
// the first return value is the index that the command will appear at
// if it's ever committed. the second return value is the current
// term. the third return value is true if this server believes it is
// the leader.
//
func (rf *Raft) Start(command interface{}) (int, int, bool) {
	index    := -1
	term     := -1
	isLeader := false

	rf.mu.Lock()
	defer rf.mu.Unlock()

	if !rf.active {
		return index, term, isLeader
	}

	index    = rf.logLen()
	term     = rf.currentTerm
	isLeader = (rf.state == Leader)

	if isLeader {
		rf.DPrintf("starting consensus for log #%d: %+v", index, Log{ command, term })

		rf.appendLogs(Log{ command, term })

		// start consensus
		rf.forAllPeers(func (peer int) { go rf.startAppendLoop(peer, term, index) })
		rf.lastHeartbeat = time.Now() // TODO
	}

	return index, term, isLeader
}

func Make(peers []*labrpc.ClientEnd, me int,
	persister *Persister, applyCh chan ApplyMsg) *Raft {
	rf := &Raft{}
	rf.peers = peers
	rf.persister = persister
	rf.me = me
	rf.applyCh = applyCh

	// initialize local state
	rf.state = Follower
	rf.currentTerm = 0

	rf.log = make([]Log, 1)
	rf.log[0] = Log{ "snapshot", 0 } // dummy start log

	rf.commitIndex = 0
	rf.lastApplied = 0

	rf.lastHeardFromLeader = time.Now()
	// rf.lastHeartbeat =

	rf.numVotes = 0

	// only for leaders, contents initialized upon winning election
	rf.nextIndex  = make([]int, len(rf.peers))
	rf.matchIndex = make([]int, len(rf.peers))

	rf.active = true

	rf.snapshotLastIndex = 0

	// initialize from state persisted before a crash
	rf.readPersist(persister.ReadRaftState())

	// start worker loops
	go rf.electionLoop()
	go rf.heartbeatLoop()
	go rf.applierLoop()

	return rf
}