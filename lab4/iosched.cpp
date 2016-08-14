#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <algorithm>
#include <cstring>

#include <unistd.h>
#include <string>

using namespace std;

class IOrequest {
  public:
    // timestamp
    int time;
    // location on the disk
    int loc;
    // id 
    int id;

    IOrequest(int ti, int loci, int idi) {
      time = ti;
      loc = loci;
      id = idi;
    }
};


class Scheduler {
  private:
    bool verbose;

  public:
    Scheduler() {
      verbose = false;
    }

    void set_verbose() {
      verbose = true;
    }

    void run(vector<IOrequest> instr) {
       
      int fin_count = 0;
      int n = instr.size();

      int tot_movement = 0;
      int turnaround = 0;
      int wait_time = 0;
      int max_wait_time = 0;
      vector<int> at(n, 0);
      vector<int> st(n, 0);
      vector<int> et(n, 0);
      
      int curr = 0;
      int prev = 0;
      
      int sim_time = 0;

      vector<IOrequest> ready;

      bool busy = false;

      IOrequest t(0, 0, 0);

      if (verbose) {
        printf("TRACE\n");
      }

      while (fin_count != n) {
        if (!busy && !ready.empty()) {
          t = this->get_next(ready, curr, (curr - prev >= 0 ? true : false));

          busy = true;
          tot_movement += abs(curr - t.loc);

          int wt = sim_time - t.time;
          wait_time += wt;
          if (wt > max_wait_time) {
            max_wait_time = wt;
          }

          if (verbose) {
            st[t.id] = sim_time;
            printf("%d:%6d issue %d %d\n", sim_time, t.id, t.loc, curr);
          }
        }

        if (busy) {
          int fin_time = sim_time + abs(curr - t.loc);

          // check if any instructions ready before current one finishes
          while (!instr.empty() && instr[0].time <= fin_time) {
            IOrequest r = this->enqueue(instr, ready, busy);

            sim_time = r.time;

            if (verbose) {
              at[r.id] = sim_time;
              printf("%d:%6d add %d\n", sim_time, r.id, r.loc);
            }
          } 

          fin_count++;

          if (t.loc != curr) {
            prev = curr;
          }
          curr = t.loc;
          sim_time = fin_time;
          busy = false;

          int tt = sim_time - t.time;
          turnaround += tt;

          if (verbose) {
            et[t.id] = sim_time;
            printf("%d:%6d finish %d\n", sim_time, t.id, tt);
          }
        } 
        else {
          IOrequest r = this->enqueue(instr, ready, busy);

          sim_time = r.time;

          if (verbose) {
            at[r.id] = sim_time;
            printf("%d:%6d add %d\n", sim_time, r.id, r.loc);
          }
        }
        if (this->update(ready)) {
          prev = curr;
        }
      }

      if (verbose) {
        printf("IOREQS INFO\n");
        for (int i = 0; i < n; ++i) {
          printf("%5d:%6d%6d%6d\n", i, at[i], st[i], et[i]);
        }
      }

      printf("SUM: %d %d %.2lf %.2lf %d\n", sim_time, tot_movement,
          (double)turnaround / fin_count, (double)wait_time / fin_count,
          max_wait_time);
    } // end schedule

    virtual bool update(vector<IOrequest>& ready) {
      return false;
    }

    virtual IOrequest enqueue(vector<IOrequest>& instr, 
                                vector<IOrequest>& ready, bool busy) {
      IOrequest t = instr.front();
      instr.erase(instr.begin());
      ready.push_back(t);
      return t;
    }

    // get the next IOrequest to process according to algo
    virtual IOrequest get_next(vector<IOrequest>&, int&, bool) = 0;
};


class Sched_FIFO: public Scheduler {
  public:
    IOrequest get_next(vector<IOrequest>& ready, int& curr, bool right) {
      IOrequest front = ready.front();
      ready.erase(ready.begin());
      return front;
    }
};


class Sched_SSTF: public Scheduler {
  public:
    IOrequest get_next(vector<IOrequest>& ready, int& curr, bool right) {
      unsigned int dis = 0xffffffff;
      int next_id = -1;

      for (int i = 0; i < ready.size(); ++i) {
        int temp = abs(curr - ready[i].loc);
        if (temp < dis) {
          dis = temp;
          next_id = i;
        }
      }
      IOrequest t = ready[next_id];
      ready.erase(ready.begin() + next_id);
      return t;
    }
};
    
bool comparator_right(IOrequest a, IOrequest b) {
  return a.loc == b.loc ? a.id < b.id : a.loc < b.loc;
}

bool comparator_left(IOrequest a, IOrequest b) {
  return a.loc == b.loc ? a.id > b.id : a.loc < b.loc;
}


class Sched_SCAN: public Scheduler {
  public:
    IOrequest get_next(vector<IOrequest>& ready, int& curr, bool right) {
      int next_id;

      if (right) {
        sort(ready.begin(), ready.end(), comparator_right);
      } 
      else {
        sort(ready.begin(), ready.end(), comparator_left);
      }

      if (right) {
        next_id = ready.size() - 1;
        
        for (int i = 0; i < ready.size(); ++i) {
          if (ready[i].loc >= curr) {
            next_id = i;
            break;
          }
        }
      } 
      else {
        next_id = 0;
        
        for (int i = ready.size() - 1; i >= 0; --i) {
          if (ready[i].loc <= curr) {
            next_id = i;
            break;
          }
        }
      }

      IOrequest t = ready[next_id];
      ready.erase(ready.begin() + next_id);
      return t;
    }
};


class Sched_CSCAN: public Scheduler{
  public:
    IOrequest get_next(vector<IOrequest>& ready, int& curr, bool right) {
      int next_id = 0;
      sort(ready.begin(), ready.end(), comparator_right);

      for (int i = 0; i < ready.size(); ++i) {
        if (ready[i].loc >= curr) {
          next_id = i;
          break;
        }
      }

      IOrequest t = ready[next_id];
      ready.erase(ready.begin() + next_id);
      return t;
    }
};


class Sched_FSCAN: public Scheduler {
  private:
    vector<IOrequest> standby;
  
  public:
    bool update(vector<IOrequest>& ready) {
      if (ready.empty()) {
        ready = standby;
        standby.clear();
        return true;
      } 
      else {
        return false;
      }
    }

    IOrequest enqueue(vector<IOrequest>& instr, vector<IOrequest>& ready, 
                        bool busy) {

      IOrequest t = instr.front();
      instr.erase(instr.begin());

      // if not busy put it into the ready queue.
      if (!busy) {
        ready.push_back(t);
      } // else put in standby
      else {
        standby.push_back(t);
      }
      return t;
    }


    IOrequest get_next(vector<IOrequest>& ready, int& curr, bool right) {
      int next_id;

      if (right) {
        sort(ready.begin(), ready.end(), comparator_right);
      } 
      else {
        sort(ready.begin(), ready.end(), comparator_left);
      }

      if (right) {
        next_id = ready.size() - 1;
        for (int i = 0; i < ready.size(); ++i) {
          if (ready[i].loc >= curr) {
            next_id = i;
            break;
          }
        }
      } 
      else {
        next_id = 0;
        for (int i = ready.size() - 1; i >= 0; --i) {
          if (ready[i].loc <= curr) {
            next_id = i;
            break;
          }
        }
      }

      IOrequest t = ready[next_id];
      ready.erase(ready.begin() + next_id);
      return t;
    }
};

void read_input_file(vector<IOrequest>& instr, FILE** f) {
  FILE *infile = *f;
  char buf[500];
  int t, l;
  int id = 0;;

  fgets(buf, 500, infile);
  while (!feof(infile)) {
    if (buf[0] != '#') {
      sscanf(buf, "%d%d", &t, &l);
      instr.push_back(IOrequest(t, l, id++));
    }
    fgets(buf, 5000, infile);
  }
}

int main(int argc, char* argv[]) {
  
  FILE* infile = NULL;
  vector<IOrequest> instr;
  Scheduler* sched = new Sched_FIFO();

  int c;
  while ((c = getopt(argc, argv, "s:")) != -1) {
    switch (c) {
       
      case 's':
        if (optarg[0] == 'i') {
          sched = new Sched_FIFO(); 
        } 
        else if (optarg[0] == 'j') {
          sched = new Sched_SSTF();
        } 
        else if (optarg[0] == 's') {
          sched = new Sched_SCAN();
        } 
        else if (optarg[0] == 'c') {
          sched = new Sched_CSCAN();
        } 
        else if (optarg[0] == 'f') {
          sched = new Sched_FSCAN();
        } 
        break;
    }
  }

  infile = fopen(argv[argc - 1], "r");

  if (infile == NULL) {
    fprintf(stderr, "Instruction file not found.\n");
    abort();
  }

  read_input_file(instr, &infile);
  //sched->set_verbose();

  sched->run(instr);

  if (infile != NULL) {
    fclose(infile);
  }
  return 0;
}
