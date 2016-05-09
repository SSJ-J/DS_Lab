package viewservice

import "net"
import "net/rpc"
import "log"
import "time"
import "sync"
import "fmt"
import "os"

type ViewServer struct {
  mu sync.Mutex
  l net.Listener
  dead bool
  me string


  // Your declarations here.
  view View     // current view
  update bool   // can view be updated?
  lastPing  map[string]time.Time
}

// Simpe Error
type Err struct {
  info string
}

func (e *Err) Error() string {
  return e.info+"\n";
}

//
// server Ping RPC handler.
//
func (vs *ViewServer) Ping(args *PingArgs, reply *PingReply) error {

  // Your code here.
  vs.lastPing[args.Me] = time.Now()
  v := &vs.view;
  if(v.Viewnum < args.Viewnum) {
    return &Err{"Viewnum bigger!"}
  }

  // Restarted primary treated as dead
  if(v.Primary == args.Me && args.Viewnum == 0) {
    v.Primary = "";
  }
 
  // update(and its condition)
  vs.update = vs.update || ((v.Primary == args.Me) && (v.Viewnum == args.Viewnum)) || (v.Primary == "")
  if(vs.update) {
    // no such node in the view
    if((args.Me != v.Primary && args.Me != v.Backup) || v.Primary == "") {
      if(v.Primary == "" && v.Backup == "") {
        v.Primary = args.Me
      } else if(v.Primary == "") {
        v.Primary = v.Backup
        v.Backup = args.Me
        if(v.Primary == v.Backup) {
          v.Backup = ""
        }
      } else if(v.Backup == "") {
        v.Backup = args.Me
      } else {
        return &Err{"more than one backups"}
      }
      v.Viewnum++
      vs.update = false
    }
  }
  reply.View = *v
  return nil
}

// 
// server Get() RPC handler.
//
func (vs *ViewServer) Get(args *GetArgs, reply *GetReply) error {

  // Your code here.
  reply.View = vs.view 
  return nil
}


//
// tick() is called once per PingInterval; it should notice
// if servers have died or recovered, and change the view
// accordingly.
//
func (vs *ViewServer) tick() {

  // Your code here.
  nt := time.Now()
  pm := &vs.view.Primary
  bk := &vs.view.Backup

  if(*pm != "" && nt.Sub(vs.lastPing[*pm]) > DeadPings * PingInterval) {
    // fmt.Println("Primary has died", *pm)
    *pm = ""
  }
  if(*bk != "" && nt.Sub(vs.lastPing[*bk]) > DeadPings * PingInterval) {
    // fmt.Println("Backup has died", *bk)
    *bk = ""
  }

}

//
// tell the server to shut itself down.
// for testing.
// please don't change this function.
//
func (vs *ViewServer) Kill() {
  vs.dead = true
  vs.l.Close()
}

func StartServer(me string) *ViewServer {
  vs := new(ViewServer)
  vs.me = me
  // Your vs.* initializations here.
  vs.view = View{0, "", ""}
  vs.update = false
  vs.lastPing = make(map[string]time.Time)

  // tell net/rpc about our RPC server and handlers.
  rpcs := rpc.NewServer()
  rpcs.Register(vs)

  // prepare to receive connections from clients.
  // change "unix" to "tcp" to use over a network.
  os.Remove(vs.me) // only needed for "unix"
  l, e := net.Listen("unix", vs.me);
  if e != nil {
    log.Fatal("listen error: ", e);
  }
  vs.l = l

  // please don't change any of the following code,
  // or do anything to subvert it.

  // create a thread to accept RPC connections from clients.
  go func() {
    for vs.dead == false {
      conn, err := vs.l.Accept()
      if err == nil && vs.dead == false {
        go rpcs.ServeConn(conn)
      } else if err == nil {
        conn.Close()
      }
      if err != nil && vs.dead == false {
        fmt.Printf("ViewServer(%v) accept: %v\n", me, err.Error())
        vs.Kill()
      }
    }
  }()

  // create a thread to call tick() periodically.
  go func() {
    for vs.dead == false {
      vs.tick()
      time.Sleep(PingInterval)
    }
  }()

  return vs
}
