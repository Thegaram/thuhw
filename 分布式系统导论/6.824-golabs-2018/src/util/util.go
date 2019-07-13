package util

import "fmt"
import "log"

func Assert(pred bool, msgs ...interface{}) {
    if !pred {
        panic(fmt.Sprintf("assertion failed", msgs...))
    }
}

func DPrintf(debug bool, format string, a ...interface{}) (n int, err error) {
	if debug {
		log.Printf(format, a...)
	}
	return
}