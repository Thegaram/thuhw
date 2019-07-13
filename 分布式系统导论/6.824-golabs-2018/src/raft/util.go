package raft

import "log"
import "fmt"

// Debugging
const Debug = 0

func DPrintf(format string, a ...interface{}) (n int, err error) {
	if Debug > 0 {
		log.Printf(format, a...)
	}
	return
}

func assert(pred bool, msgs ...interface{}) {
	if !pred {
		panic(fmt.Sprintf("assertion failed", msgs...))
	}
}

func min(a, b int) int {
    if a < b { return a } else { return b }
}

func max(a, b int) int {
    if a > b { return a } else { return b }
}
