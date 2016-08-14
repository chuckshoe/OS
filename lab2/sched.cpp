#include <iostream>
#include <fstream>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <unistd.h>

using namespace std;

class Process {
  private:
    int pid;
    // rem: remaining time to finish, 
    // remcb: remaining CPU burst for the process (RR, PRIO only)
    // last_ready: last time the process is in process status
    int rem, remcb, last_ready;
    // static and dynamic priority
    int s_prio; 
    int d_prio; 
    
    int at, tc, cb, io;
    int ft, it, cw;
  public:
    int state_time;
    Process() {
      pid = 0;
      s_prio = d_prio = remcb = last_ready = ft = 0;
      it = cw = rem = at = tc = cb = io = 0;
    }

    Process(int id, int a, int t, int c, int i) {
      pid = id;
      at = a;
      rem = tc = t;
      cb = c;
      io = i;
      state_time = at;
      remcb = last_ready = ft = it = cw = 0;
    }

    void set_pid(int id) {
      pid = id;
    }

    int get_pid() {
      return pid;
    }

    void set_sprio(int a) {
      s_prio = a;
    }        
    
    int get_sprio() {
      return s_prio;
    }

    void set_dprio(int a) {
      d_prio = a;
    }        
    
    int get_dprio() {
      return d_prio;
    }
    
    void set_at(int a) {
      at = a;
    }        

    int get_at() {
      return at;
    }

    void set_tc(int t) {
      tc = t;
    }

    int get_tc() {
      return tc;
    }

    void set_cb(int c) {
      cb = c;
    }

    int get_cb() {
      return cb;
    }

    void set_io(int i) {
      io = i;
    }

    int get_io() {
      return io;
    }

    void set_ft(int f) {
      ft = f;
    }

    int get_ft() {
      return ft;
    }

    void set_it(int i) {
      it = i;
    }

    int get_it() {
      return it;
    }

    void set_cw(int c) {
      cw = c;
    }

    int get_cw() {
      return cw;
    }

    void set_rem(int r) {
      rem = r;
    }

    int get_rem() {
      return rem;
    }

    void set_remcb(int r) {
      remcb = r;
    }       

    int get_remcb() {
      return remcb;
    }      

    void set_last_ready(int l) {
      last_ready = l;
    }

    int get_last_ready() {
      return last_ready;
    }
};

class Event {
  public:
    int launch_time;

    int create_time;

    int pid;

    // the trasition defines the event 0, 1, 2, 3, 4, 5
    // 0: arrive -> ready
    // 1:running -> block 
    // 2: running -> ready 
    // 3: ready -> running
    // 4: block -> ready
    // 5: done
    int transition;

    Event(int t, int c, int p, int tr) {
      launch_time = t;
      create_time = c;
      pid = p;
      transition = tr;
    }
};

class Scheduler {
  protected:
    // scheduler type 0:FCFS, 1:LCFS, 2:SJF, 3:RR, 4:PRIO
    int type;

    vector<Process> readyq;
  public:
    
    void set_type(int t) {
      type = t;
    }

    int get_type() {
      return type;
    }

    virtual int get_quantum() {
      return 0;
    }

    virtual string get_name() = 0;

    virtual Process get_process() = 0;

    virtual void put_process(Process) = 0;
   
    //  for priority scheduler
    virtual void add_expired_process(Process) = 0;
};

class FCFSScheduler : public Scheduler {
  public:
    FCFSScheduler();

    FCFSScheduler(int t) {
      type = t;
    }

    string get_name() {
      return "FCFS";
    }

    Process get_process() {
      Process ret;
      if (!readyq.empty()) {
        ret = readyq.front();
        readyq.erase(readyq.begin());
        return ret;
      }
      else {
        Process p (-1, -1, -1, -1, -1);
        return p;
      }
    }

    void put_process(Process p) {
      readyq.push_back(p);
    }
    
    void add_expired_process(Process) {
    }
};

class LCFSScheduler : public Scheduler {
  public:
    LCFSScheduler();

    LCFSScheduler(int t) {
      type = t;
    }

    string get_name() {
      return "LCFS";
    }

    Process get_process() {
      Process ret;
      if (!readyq.empty()) {
        ret = readyq.back();
        readyq.pop_back();
        return ret;
      }
      else {
        Process p (-1, -1, -1, -1, -1);
        return p;
      }
    }

    void put_process(Process p) {
      readyq.push_back(p);
    }
    void add_expired_process(Process) {
    }
};

class SJFScheduler : public Scheduler {
  public:
    SJFScheduler();

    SJFScheduler(int t) {
      type = t;
    }

    string get_name() {
      return "SJF";
    }

    Process get_process() {
      Process ret;
      if (!readyq.empty()) {
        int min = readyq[0].get_rem();
        int ch = 0;
        for (int i = 1; i < readyq.size(); ++i) {
          if (readyq[i].get_rem() < min) {
            min = readyq[i].get_rem();
            ch = i;
          }
        }
        ret = readyq[ch];
        readyq.erase(readyq.begin() + ch);
        return ret;
      }
      else {
        Process p (-1, -1, -1, -1, -1);
        return p;
      }
    }

    void put_process(Process p) {
      readyq.push_back(p);
    }
    void add_expired_process(Process) {
    }
};

class RRScheduler : public Scheduler {
  private:
    int quantum;
  public:
    RRScheduler();

    RRScheduler(int t, int q) {
      type = t;
      quantum = q;
    }

    int get_quantum() {
      return quantum;
    }

    string get_name() {
      return "RR";
    }

    Process get_process() {
      Process ret;
      if (!readyq.empty()) {
        ret = readyq.front();
        readyq.erase(readyq.begin());
        return ret;
      }
      else {
        Process p (-1, -1, -1, -1, -1);
        return p;
      }
    }

    void put_process(Process p) {
      readyq.push_back(p);
    }
    void add_expired_process(Process) {
    }
};

class PRIOScheduler : public Scheduler {
  private:
    int quantum;
    vector<Process> active0;
    vector<Process> active1;
    vector<Process> active2;
    vector<Process> active3;

    vector<Process> expired0;
    vector<Process> expired1;
    vector<Process> expired2;
    vector<Process> expired3;

  public:
    PRIOScheduler();

    PRIOScheduler(int t, int q) {
      type = t;
      quantum = q;
    }

    int get_quantum() {
      return quantum;
    }

    string get_name() {
      return "PRIO";
    }

    Process get_process() {
      Process ret;
      if(active0.empty() && active1.empty() && 
          active2.empty() && active3.empty()) {
          active0.swap(expired0);
          active1.swap(expired1);
          active2.swap(expired2);
          active3.swap(expired3);
      }

      if(active3.size()!=0) {
          ret = active3.front();
          active3.erase(active3.begin());
          return ret;     
      }
      else if(active2.size()!=0) {
          ret = active2.front();
          active2.erase(active2.begin());
          return ret;     
      }
      else if(active1.size()!=0) {
          ret = active1.front();
          active1.erase(active1.begin());
          return ret;     
      }
      else if(active0.size()!=0) {
          ret = active0.front();
          active0.erase(active0.begin());
          return ret;     
      }
      else {
        Process p (-1, -1, -1, -1, -1);
        return p;
      }
    }

    void put_process(Process p) {      
      if(p.get_dprio()==0)
          active0.push_back(p);
      else if(p.get_dprio()==1)
          active1.push_back(p);
      else if(p.get_dprio()==2)
          active2.push_back(p);
      else if(p.get_dprio()==3)
          active3.push_back(p);
    }

    void add_expired_process(Process p) {
        if(p.get_dprio()==0)
            expired0.push_back(p);
        else if(p.get_dprio()==1)
            expired1.push_back(p);
        else if(p.get_dprio()==2)
            expired2.push_back(p);
        else if(p.get_dprio()==3)
            expired3.push_back(p);
    }
};

typedef struct io_interval {
  int beg;
  int end;
} io_interval;


Scheduler* scheduler;
vector<Event> events;
vector<Process> proc;
vector<io_interval> intervals;
vector<int> rand_nums;
int ofs=0;
int count_random=0;
bool verbose = false;

int myrandom(int burst) { 
  int t = 1 + (rand_nums[ofs] % burst); 
  ofs++;
  if(ofs==count_random)
    ofs=0;
  return t;
}

void read_random_file(string s) {
  ifstream fin;
  fin.open(s.c_str());
  string line;

  if (fin.is_open()) {
    getline(fin,line);
    count_random = atoi(line.c_str());
    while (getline(fin, line)) {
      rand_nums.push_back(atoi(line.c_str()));
    }
  }
  else {
    cout<<"Unable to open file"<<endl; 
  }
  fin.close();
}

void read_process_file(string s) {
  ifstream fin;
  fin.open(s.c_str());
  
  if (fin.is_open()) { 
    string line;
    int i=0;
    while (getline(fin, line)) {
      string temp;
      stringstream tokens(line);
      int at,tc,cb,io; 

      tokens>>temp;
      at = atoi(temp.c_str());
      tokens>>temp;
      tc = atoi(temp.c_str());
      tokens>>temp;
      cb = atoi(temp.c_str());
      tokens>>temp;
      io = atoi(temp.c_str());

      Process t(i++,at,tc,cb,io);
      proc.push_back(t);
    }
  }
  else {
    cout << "Unable to open file"<<endl;
  }
  fin.close();
}

Scheduler* get_scheduler(char *stype)
{
  if(stype[0] == 'F')
    return new FCFSScheduler(0);
  else if(stype[0] == 'L')
    return new LCFSScheduler(1);
  else if(stype[0] == 'S')
    return new SJFScheduler(2);
  else if(stype[0]== 'P' || stype[0] == 'R') {
    int count = 0;
    int i = 1;
    while(stype[i]!='\0') {
      count++;
      i++;
    }
    char q[count+1];
    q[count] = '\0';
    for(int j=1;j<=count;j++) {
      q[j-1] = stype[j];
    }
    int quantum = atoi(q);
    if(stype[0] == 'P') {
      return new PRIOScheduler(4, quantum);
    }
    else {
      return new RRScheduler(3, quantum);
    }
  }
}

void put_event(Event e) {
  int loc = events.size();
  
  for (int i = 0; i < events.size(); ++i) {
    if (e.launch_time < events[i].launch_time) {
      loc = i;
      break;
    }
  }
  events.insert(events.begin() + loc, e);
}

Event get_event() {
  Event e(0, 0, 0, 0);
  if (!events.empty()) {
    e = events.front();
    //events.erase(events.begin());
  }
  return e;
}

void print_verbose(int cur, Event e, Process p, int burst) {
  if (verbose) {

    printf("%d %d %d:", cur, 
        p.get_pid(), cur - p.state_time);
    
    // transition is Running -> Block
    if (e.transition == 1) {
      printf(" RUNNG -> BLOCK  ib=%d rem=%d\n", 
           burst, p.get_rem());
    } // transition is Running -> Ready
    else if (e.transition == 2) {
      printf(" RUNNG -> READY  cb=%d rem=%d prio=%d\n", 
           burst, p.get_rem(), p.get_dprio());
    } // transition is Ready -> Running
    else if (e.transition == 3) {
      printf(" READY -> RUNNG cb=%d rem=%d prio=%d\n", 
           burst, p.get_rem(), p.get_dprio());
    } // event is done
    else if (e.transition == 5) {
      printf(" Done\n");
    } 
    else if (e.transition == 0) {
      printf(" CREATED -> READY\n");
    } 
    else if (e.transition == 4) {
      printf(" BLOCK -> READY\n");
    } 
  }
}

void init_events() { 
  for (int i = 0; i < proc.size(); ++i) {
    put_event(Event(proc[i].get_at(), proc[i].get_at(),proc[i].get_pid(), 0));
    proc[i].set_sprio(myrandom(4));
    proc[i].set_dprio(proc[i].get_sprio()-1);
  }
}

// own comparator for intervals 
bool cmp(io_interval a, io_interval b) {
  return a.beg == b.beg ? a.end < b.end : a.beg < b.beg;
}

void run_simulation() {

  Event  curr_event(0, 0, 0, 0);

  int i    = curr_event.pid;
  int ib   = 0;
  int cb   = 0;
  
  int sim_time = proc[0].get_at();

  bool call_sched = false;
  bool isp_running =  false;

  while (!events.empty()) {
    curr_event = get_event();
    //if (sim_time < curr_event.launch_time) {
      sim_time = curr_event.launch_time;
    //}
    i = curr_event.pid;

    // arrive -> ready
    if (curr_event.transition == 0) {

      print_verbose(sim_time, curr_event, proc[i], 0);
      proc[i].set_last_ready(sim_time);
      scheduler->put_process(proc[i]);
      call_sched = true;
      proc[i].state_time = sim_time;
      
    } // running -> block
    else if (curr_event.transition == 1) {

      ib = myrandom( proc[i].get_io() );
      print_verbose(sim_time, curr_event, proc[i], ib);
      Event e(sim_time + ib, sim_time, i, 4);
      put_event(e);

      // create new IO interval
      io_interval io;
      io.beg = sim_time;
      io.end = sim_time + ib;
      intervals.push_back(io);

      proc[i].set_it(proc[i].get_it() + ib);
      call_sched = true;
      isp_running = false;
      proc[i].state_time = sim_time;

    } // running -> ready (preempt)
    else if (curr_event.transition == 2) {

      print_verbose(sim_time, curr_event, proc[i], proc[i].get_remcb());
      proc[i].set_last_ready(sim_time);

      if (scheduler->get_name() == "PRIO") {
        proc[i].set_dprio(proc[i].get_dprio()-1);
        if (proc[i].get_dprio() == -1) {
          proc[i].set_dprio(proc[i].get_sprio()-1);
          scheduler->add_expired_process(proc[i]);
        }
        else {
          scheduler->put_process(proc[i]);
        } 
      }
      else {
        scheduler->put_process(proc[i]);
      }
      call_sched = true;
      isp_running = false;
      proc[i].state_time = sim_time;
    } // ready -> running
    else if (curr_event.transition == 3) {
      proc[i].set_cw(proc[i].get_cw() + sim_time - proc[i].get_last_ready());
      isp_running = true;
      if (scheduler->get_name() == "RR" || scheduler->get_name() == "PRIO") {
        int q = scheduler->get_quantum();
        // if CPU burst has expired, generate CPU burst
        if (proc[i].get_remcb() == 0) {
          proc[i].set_remcb( myrandom(proc[i].get_cb()) );
        }
        cb = proc[i].get_remcb();

        // two cases: quantum < remaining CPU burst or vice-versa
        if (q <= cb) {
          // process will finish within the quantum
          if (proc[i].get_rem() <= q) {
            print_verbose(sim_time, curr_event, proc[i], proc[i].get_rem());

            Event e(sim_time + proc[i].get_rem(), sim_time, i, 5);
            put_event(e);

            proc[i].set_rem(0);
            
          } // process will not be finished
          else {
            print_verbose(sim_time, curr_event, proc[i], cb);

            // if the quantum is equal to CB, then the burst
            // will be used up and process will be blocked
            // Otherwise the process will be preempted
            Event e(sim_time + q, sim_time, i, cb == q ? 1 : 2);
            put_event(e);

            proc[i].set_remcb(cb - q);
            proc[i].set_rem(proc[i].get_rem() - q);
          }
          
        } // quantum > remaining CPU burst
        else {
          // process will finish within the CPU burst
          if (proc[i].get_rem() <= cb) {
            print_verbose(sim_time, curr_event, proc[i], proc[i].get_rem());

            Event e(sim_time + proc[i].get_rem(), sim_time, i, 5);
            put_event(e);

            proc[i].set_rem(0);
            
          } // process will be blocked
          else {
            print_verbose(sim_time, curr_event, proc[i], cb);

            Event e(sim_time + cb, sim_time, i, 1);
            put_event(e);

            proc[i].set_rem(proc[i].get_rem() - cb);
          }
          proc[i].set_remcb(0);
        }

      } // not RR or PRIO scheduler
      else {
        // generate CPU burst
        proc[i].set_remcb(myrandom( proc[i].get_cb()));
        cb = proc[i].get_remcb();

        // process will finish within the CPU burst
        if (proc[i].get_rem() <= cb) {
          print_verbose(sim_time, curr_event, proc[i], proc[i].get_rem());

          Event e(sim_time + proc[i].get_rem(), sim_time, i, 5);
          put_event(e);

          proc[i].set_rem(0);
          
        } // process will be blocked
        else {
          print_verbose(sim_time, curr_event, proc[i], cb);

          Event e(sim_time + cb, sim_time, i, 1);
          put_event(e);

          proc[i].set_rem(proc[i].get_rem() - cb);
        }
      }
      proc[i].state_time = sim_time;
      
    } // block -> ready
    else if (curr_event.transition == 4) {

      print_verbose(sim_time, curr_event, proc[i], 0);
      
      proc[i].set_dprio(proc[i].get_sprio()-1);
      proc[i].set_last_ready(sim_time);

      scheduler->put_process(proc[i]);
      call_sched = true;
      proc[i].state_time = sim_time;
      
    } // done
    else if (curr_event.transition == 5) {

      proc[i].set_ft(sim_time);
      print_verbose(sim_time, curr_event, proc[i], 0);
      call_sched = true;
      isp_running = false;
      proc[i].state_time = sim_time;

    }
    
    events.erase(events.begin());
    if (call_sched) {
      if (!events.empty() && events.front().launch_time == sim_time) {
        continue;
      }
      call_sched = false;
      Process p;
      if (isp_running == false) {
        p = scheduler->get_process();
        if (p.get_pid() == -1) {
          continue;
        }
        Event e(sim_time, sim_time, p.get_pid(), 3);
        put_event(e);
      }
    }
  }
}

void print_results() {

  int    last_finish = 0;
  double turnaround = 0.0;
  double cpu_util = 0.0;
  double io_util = 0.0;
  double cpu_wait = 0.0; 

  // sort io events and calculate entire io utilization
  sort(intervals.begin(), intervals.end(), cmp);
  int end;
  for (int i = 0; i < intervals.size(); ++i) {
    if (i == 0 || end <= intervals[i].beg) {
      io_util += intervals[i].end - intervals[i].beg;
      end = intervals[i].end;
    } 
    else if (end < intervals[i].end) {
      io_util += intervals[i].end - end;
      end = intervals[i].end;
    }
  }

  cout << scheduler->get_name();
  if (scheduler->get_name() == "RR" || scheduler->get_name() == "PRIO") {
    cout << " " << scheduler->get_quantum();
  }
  printf("\n");
  for (int i = 0; i < proc.size(); ++i) {
    Process p = proc[i];
    printf("%04d: %4d %4d %4d %4d %1d | %5d %5d %5d %5d\n",
        p.get_pid(), p.get_at(), p.get_tc(), p.get_cb(), 
        p.get_io(), p.get_sprio(), p.get_ft(), p.get_ft() - p.get_at(), 
        p.get_it(), p.get_cw());
    if (p.get_ft() > last_finish) {
      last_finish = p.get_ft();
    }
    turnaround += p.get_ft() - p.get_at();
    cpu_wait += p.get_cw();
    cpu_util += p.get_tc();
  }
  printf("SUM: %d %.2lf %.2lf %.2lf %.2lf %.3lf\n", last_finish,
      cpu_util * 100 / last_finish, io_util * 100 / last_finish,
      turnaround / proc.size(), cpu_wait / proc.size(),
      (double)proc.size() * 100 / last_finish);

}

int main(int argc, char* argv[]) {

  int c;
  opterr = 0;
  char *svalue;
  while ((c = getopt (argc, argv, "vs:")) != -1)
    switch (c) {
      case 'v' :
        verbose = true;
        break;

      case 's' :
        svalue = optarg;
        break;
      default:
        abort ();
    }

  scheduler = get_scheduler(svalue);
  string path1 = argv[optind+1];
  read_random_file(path1);
  string path2 = argv[optind];
  read_process_file(path2);
  
  init_events();
  run_simulation();
  print_results();

  return 0;
}
