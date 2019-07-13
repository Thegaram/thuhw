package mapreduce

import "fmt"



func feedTasks(ntasks int, taskChan chan int) {
	for task := 0; task < ntasks; task++ {
		taskChan <- task
	}
}

func countSuccesses(ntasks int, successChan chan int, taskChan chan int) {
	numTasksToGo := ntasks
	for range successChan {
		if numTasksToGo -= 1; numTasksToGo == 0 {
			close(taskChan)
			break
		}
	}
}

//
// schedule() starts and waits for all tasks in the given phase (mapPhase
// or reducePhase). the mapFiles argument holds the names of the files that
// are the inputs to the map phase, one per map task. nReduce is the
// number of reduce tasks. the registerChan argument yields a stream
// of registered workers; each item is the worker's RPC address,
// suitable for passing to call(). registerChan will yield all
// existing registered workers (if any) and new ones as they register.
//
func schedule(jobName string, mapFiles []string, nReduce int, phase jobPhase, registerChan chan string) {
	var ntasks int
	var n_other int // number of inputs (for reduce) or outputs (for map)
	switch phase {
	case mapPhase:
		ntasks = len(mapFiles)
		n_other = nReduce
	case reducePhase:
		ntasks = nReduce
		n_other = len(mapFiles)
	}

	fmt.Printf("Schedule: %v %v tasks (%d I/Os)\n", ntasks, phase, n_other)

	// All ntasks tasks have to be scheduled on workers. Once all tasks
	// have completed successfully, schedule() should return.
	//
	// Your code here (Part III, Part IV).
	//

	taskChan    := make(chan int)
	successChan := make(chan int)

	scheduleSingle := func(worker string, task int) {
		file := ""
		if phase == mapPhase { file = mapFiles[task] }
		command := DoTaskArgs{jobName, file, phase, task, n_other}

		success := call(worker, "Worker.DoTask", command, nil)

		if success {
			successChan <- task
			registerChan <- worker // this will block on last task (no one to read)
		} else {
			taskChan <- task
		}
	}

	go feedTasks(ntasks, taskChan)
	go countSuccesses(ntasks, successChan, taskChan)

	for task := range taskChan {
		worker := <-registerChan
		go scheduleSingle(worker, task)
	}

	fmt.Printf("Schedule: %v done\n", phase)
}
