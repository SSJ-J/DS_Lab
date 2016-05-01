package mapreduce
import "container/list"
import "fmt"
import "sync"

type WorkerInfo struct {
  address string
  // You can add definitions here.
}


// Clean up all workers by sending a Shutdown RPC to each one of them Collect
// the number of jobs each work has performed.
func (mr *MapReduce) KillWorkers() *list.List {
  l := list.New()
  for _, w := range mr.Workers {
    DPrintf("DoWork: shutdown %s\n", w.address)
    args := &ShutdownArgs{}
    var reply ShutdownReply;
    ok := call(w.address, "Worker.Shutdown", args, &reply)
    if ok == false {
      fmt.Printf("DoWork: RPC %s shutdown error\n", w.address)
    } else {
      l.PushBack(reply.Njobs)
    }
  }
  return l
}

func (mr *MapReduce) RunMaster() *list.List {
    // Your code here
    // get new worker
    worker_ch := make(chan string, 10)
    var wg sync.WaitGroup

    // get new worker
    go func() {
        for mr.alive {
            worker_ch <- (<-mr.registerChannel)
        }
    } ()

    // do map
    for i := 0; i < mr.nMap; i++ {
        wg.Add(1)   // increment the WaitGroup counter
        args := &DoJobArgs{mr.file, "Map", i, mr.nReduce}
        var reply DoJobReply

        // distribute map tasks
        go func() {
            ok := false
            var worker string
            for !ok {   // fault-tolerance(do it again)
                worker = <-worker_ch
                ok = call(worker, "Worker.DoJob", args, &reply)
            }
            worker_ch <- worker
            wg.Done()   // decrement the counter
        }()
    }
    wg.Wait()
    DPrintf("map complete!\n")

    // do reduce
    for i := 0; i < mr.nReduce; i++ {
        wg.Add(1)   // increment the WaitGroup counter
        args := &DoJobArgs{mr.file, "Reduce", i, mr.nMap}
        var reply DoJobReply

        // distribute map tasks
        go func() {
            ok := false
            var worker string
            for !ok {   // fault-tolerance(do it again)
                worker = <-worker_ch
                ok = call(worker, "Worker.DoJob", args, &reply)
            }
            worker_ch <- worker
            wg.Done()   // decrement the counter
        }()
    }
    wg.Wait()

    return mr.KillWorkers()
}











