package mapreduce

import (
	"encoding/json"
	"log"
	"os"
	"sort"
)

func extractValues(kvs []KeyValue) []string {
    values := make([]string, len(kvs))

    for i, kv := range kvs {
        values[i] = kv.Value
    }

    return values
}

func readResults(jobName string, reduceTask int, nMap int) []KeyValue {
	results := make([]KeyValue, 0)

	for m := 0; m < nMap; m++ {
		filename := reduceName(jobName, m, reduceTask)
		file, err := os.Open(filename)
		HandleError(err, "Unable to open file", filename)
		defer file.Close()

		dec := json.NewDecoder(file)
		var nextValue KeyValue

		for err = dec.Decode(&nextValue); err == nil; err = dec.Decode(&nextValue) {
			results = append(results, nextValue)
		}
	}

	return results
}

func doReduce(
	jobName string, // the name of the whole MapReduce job
	reduceTask int, // which reduce task this is
	outFile string, // write the output here
	nMap int, // the number of map tasks that were run ("M" in the paper)
	reduceF func(key string, values []string) string,
) {
	//
	// doReduce manages one reduce task: it should read the intermediate
	// files for the task, sort the intermediate key/value pairs by key,
	// call the user-defined reduce function (reduceF) for each key, and
	// write reduceF's output to disk.
	//
	// You'll need to read one intermediate file from each map task;
	// reduceName(jobName, m, reduceTask) yields the file
	// name from map task m.
	//
	// Your doMap() encoded the key/value pairs in the intermediate
	// files, so you will need to decode them. If you used JSON, you can
	// read and decode by creating a decoder and repeatedly calling
	// .Decode(&kv) on it until it returns an error.
	//
	// You may find the first example in the golang sort package
	// documentation useful.
	//
	// reduceF() is the application's reduce function. You should
	// call it once per distinct key, with a slice of all the values
	// for that key. reduceF() returns the reduced value for that key.
	//
	// You should write the reduce output as JSON encoded KeyValue
	// objects to the file named outFile. We require you to use JSON
	// because that is what the merger than combines the output
	// from all the reduce tasks expects. There is nothing special about
	// JSON -- it is just the marshalling format we chose to use. Your
	// output code will look something like this:
	//
	// enc := json.NewEncoder(file)
	// for key := ... {
	// 	enc.Encode(KeyValue{key, reduceF(...)})
	// }
	// file.Close()
	//
	// Your code here (Part I).
	//

	results := readResults(jobName, reduceTask, nMap)

	if len(results) == 0 {
		log.Fatal("Empty results in doReduce")
	}

	sort.Slice(results, func(ii, jj int) bool {
		return results[ii].Key < results[jj].Key
	})

	file, err := os.Create(outFile)
	HandleError(err, "Unable to create file", outFile)
	defer file.Close()
	enc := json.NewEncoder(file)

	from := 0
	key  := results[0].Key

	for ii, kv := range results {
		if kv.Key != key {
			values := extractValues(results[from:ii])
			enc.Encode(KeyValue{key, reduceF(key, values)})

			from = ii
			key = kv.Key
		}
	}

	values := extractValues(results[from:])
	enc.Encode(KeyValue{key, reduceF(key, values)})
}
