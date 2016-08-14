#include <iostream>
#include <fstream>
#include <cstdio>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <unistd.h>

using namespace std;

/* 
 * page table entry(pte):
 * 
 * 0       1        2         3|4...31
 * Present Modified Reference Pagedout|addr
 */

void set_present_bit(unsigned int& p) {
    unsigned int mask = 0x80000000;
    p |= mask;
}

void clear_present_bit(unsigned int& p) {
    unsigned int mask = 0x7fffffff;
    p &= mask;
}

unsigned int get_present_bit(unsigned int p) {
    return ((p >> 31) | 0);
}

void set_modified_bit(unsigned int& p) {
    unsigned int mask = 0x40000000;
    p |= mask;
}

void clear_modified_bit(unsigned int& p) {
    unsigned int mask = 0xbfffffff;
    p &= mask;
}

unsigned int get_modified_bit(unsigned int p) {
    return ((p >> 30) & 1) | 0;
}

void set_referenced_bit(unsigned int& p) {
    unsigned int mask = 0x20000000;
    p |= mask;
}

void clear_referenced_bit(unsigned int& p) {
    unsigned int mask = 0xdfffffff;
    p &= mask;
}

unsigned int get_referenced_bit(unsigned int p) {
    return ((p >> 29) & 1) | 0;
}

void set_pagedout_bit(unsigned int& p) {
    unsigned int mask = 0x10000000;
    p |= mask;
}

void clear_pagedout_bit(unsigned int& p) {
    unsigned int mask = 0xefffffff;
    p &= mask;
}

unsigned int get_pagedout_bit(unsigned int p) {
    return ((p >> 28) & 1) | 0;
}

void set_frame_number(unsigned int& p, int f) {
    p = (p & 0xf0000000) | f;
}

unsigned int get_frame_number(unsigned int p) {
    return ((p << 4) >> 4);
}


class Pager {
  protected:
    vector<int> rand_nums;
    int count_random;
    int num_of_frames;
    int ofs;

  public:
    void set_random(vector<int> r, int c) {
      rand_nums = r;
      count_random = c;
      ofs = 0;
    }
    
    int get_random_number() { 
      int t = rand_nums[ofs]; 
      ofs++;
      if(ofs==count_random)
        ofs=0;
      return t;
    }

    void set_num_of_frames(int f) {
      num_of_frames = f;
    }

    virtual void update(vector<unsigned int>& list, unsigned int i) {
    }

    virtual int get_frame(vector<unsigned int>&, 
                          vector<unsigned int>&, vector<unsigned int>&) = 0;

};

class Pager_NRU : public Pager {
  private:
    // when counter ticks 10, clear all R bits
    int counter;
  public:
    int get_frame(vector<unsigned int>& pages,
                  vector<unsigned int>& frames, 
                  vector<unsigned int>& rev_frames) {

      vector<vector<int> > priority(4, vector<int>());

      for (int i = 0; i < pages.size(); ++i) {
        if (get_present_bit(pages[i]) == 1) {
          unsigned int rbit = get_referenced_bit(pages[i]);
          unsigned int mbit = get_modified_bit(pages[i]);

          if (rbit == 1 && mbit == 1) {
            priority[3].push_back(get_frame_number(pages[i]));
          } 
          else if (rbit == 1 && mbit == 0) {
            priority[2].push_back(get_frame_number(pages[i]));
          } 
          else if (rbit == 0 && mbit == 1) {
            priority[1].push_back(get_frame_number(pages[i]));
          } 
          else {
            priority[0].push_back(get_frame_number(pages[i]));
          }
        }
      }

      int ret = -1;
      for (int i = 0; i < 4; ++i) {
        if (priority[i].size() > 0) {
          ret = priority[i][get_random_number() % priority[i].size()];
          break;
        }
      }

      ++counter;
      // when ticks 10, clean all reference bits
      if (counter == 10) {
        counter = 0;
        for (int i = 0; i < pages.size(); ++i) {
          if (get_present_bit(pages[i]) == 1) {
            clear_referenced_bit(pages[i]);
          }
        }
      }

      return ret;
    }
};

class Pager_LRU : public Pager {
  public:
    // LRU has its own update function
    void update(vector<unsigned int>& frames, unsigned int f) {
      for (int i = 0; i < frames.size(); ++i) {
        if (frames[i] == f) {
          frames.erase(frames.begin() + i);
          break;
        }
      }
      frames.push_back(f);
    }


    int get_frame(vector<unsigned int>& pages, 
                  vector<unsigned int>& frames,
                  vector<unsigned int>& rev_frames) {
      unsigned int f = frames.front();
      frames.erase(frames.begin());

      frames.push_back(f);
      return frames.back();
    }
};

class Pager_Random : public Pager {
  public:
    int get_frame(vector<unsigned int>& pages,
                  vector<unsigned int>& frames,
                  vector<unsigned int>& rev_frames) {
      int r = get_random_number();
      return frames[r % frames.size()];
    }
};

class Pager_FIFO : public Pager {
  public:
    int get_frame(vector<unsigned int>& pages,
                  vector<unsigned int>& frames,
                  vector<unsigned int>& rev_frames) {

      unsigned int f = frames.front();
      frames.erase(frames.begin());
      frames.push_back(f);
      return f;
    }
};

class Pager_SecondChance : public Pager {
  public:
    int get_frame(vector<unsigned int>& pages,
                  vector<unsigned int>& frames,
                  vector<unsigned int>& rev_frames) {

      bool flag = false;
      unsigned int f, pi;

      while (flag == false) {
        f = frames.front();
        pi = rev_frames[f];
        if (get_referenced_bit(pages[pi]) == 1) {
          clear_referenced_bit(pages[pi]);
          frames.erase(frames.begin());
          frames.push_back(f);
        } 
        else {
          flag = true;
          frames.erase(frames.begin());
          frames.push_back(f);
        }
      }
      return frames.back();
    }
};

class Pager_Clock_P : public Pager {
  private:
    // used to record the location
    int counter = 0;
  public:
    int get_frame(vector<unsigned int>& pages,
                  vector<unsigned int>& frames,
                  vector<unsigned int>& rev_frames) {
      bool flag = false;
      unsigned int f, pi;

      while (flag == false) {
        f = frames[counter];
        pi = rev_frames[f];

        if (get_referenced_bit(pages[pi]) == 1) {
          clear_referenced_bit(pages[pi]);
        } 
        else {
          flag = true;
        }
        
        counter = (counter + 1) % frames.size();
      }
      return f;
    }
};

class Pager_Clock_V : public Pager {
  private:
    // used to record
    int counter = 0;
  public:
    int get_frame(vector<unsigned int>& pages,
                  vector<unsigned int>& frames,
                  vector<unsigned int>& rev_frames) {
      bool flag = false;
      unsigned int ret;

      while (flag == false) {
        if (get_present_bit(pages[counter]) == 1) {
           
          if (get_referenced_bit(pages[counter]) == 1) {
            clear_referenced_bit(pages[counter]);
          } 
          else {
            flag = true;
            ret = get_frame_number(pages[counter]);
          }
        }
        
        counter = (counter + 1) % pages.size();
      }

      return ret;
    }
};

class Pager_Aging_P : public Pager {
  private:
    // vector to record the ages
    vector<unsigned int> ages;
  public:
    int get_frame(vector<unsigned int>& pages,
                  vector<unsigned int>& frames,
                  vector<unsigned int>& rev_frames) {

      unsigned int min_age = 0xffffffff;
      unsigned int min_frame = -1;
      unsigned int min_index = -1;

      if (ages.size() == 0) {
        ages = vector<unsigned int>(num_of_frames, 0);
      }

      for (int i = 0; i < frames.size(); ++i) {

        ages[i] = (ages[i] >> 1) | 
          ((get_referenced_bit(pages[rev_frames[frames[i]]])) << 31);

        if (ages[i] < min_age) {
          min_age = ages[i];
          min_frame = frames[i];
          min_index = i;
        }

        clear_referenced_bit(pages[rev_frames[frames[i]]]);

      }
      
      ages[min_index] = 0;

      return min_frame;
    }
};

class Pager_Aging_V : public Pager {
  private:
    // vector to record the ages
    vector<unsigned int> ages;
  public:
    int get_frame(vector<unsigned int>& pages,
                  vector<unsigned int>& frames,
                  vector<unsigned int>& rev_frames) {

      unsigned int min_age = 0xffffffff;
      unsigned int min_index = -1;

      if (ages.size() == 0) {
        ages = vector<unsigned int>(64, 0);
      }

      for (int i = 0; i < pages.size(); ++i) {
        ages[i] = (ages[i] >> 1) | 
          ((get_referenced_bit(pages[i])) << 31);

        if (get_present_bit(pages[i]) == 1 && ages[i] < min_age) {
          min_age = ages[i];
          min_index = i;
        }

        if (get_present_bit(pages[i]) == 1) {
          clear_referenced_bit(pages[i]);
        }
      }
      
      ages[min_index] = 0;

      return get_frame_number(pages[min_index]);
    }
};


class VMM {
  private:
   
    unsigned int frame_limit; // max no. of frames
    
    vector<unsigned int> frames; // frame list

    vector<unsigned int> rev_frames; // frame table
   
    vector<unsigned int> pages; // page table

    int cnt_inst, cnt_unmap, cnt_map, cnt_in, cnt_out, cnt_zero; // counters

    Pager* algo; // page replacement algo

    bool O, P, F, S, p, f, a;

  public:
    VMM(Pager* in_algo, unsigned int num_of_frames, bool iO, bool iP, 
        bool iF, bool iS, bool ip, bool iif, bool ia) {

      frame_limit = num_of_frames;

      cnt_inst = cnt_unmap = cnt_map = cnt_in = cnt_out = cnt_zero = 0;

      algo = in_algo;

      O = iO; P = iP; F = iF; S = iS;
      p = ip; f = iif, a = ia;

      pages = vector<unsigned int>(64, 0);

      rev_frames = vector<unsigned int>(num_of_frames, -1);

    }


    void map_page_frame(unsigned int rw, unsigned int pi) {
      unsigned int& pte = pages[pi];

      if (O) {
        printf("==> inst: %d %d\n", rw, pi);
      }
      // has not been mapped 
      if (get_present_bit(pte) == 0) {

        unsigned int frame_number;
        // still free frame available
        if (frames.size() < frame_limit) {
          frame_number = frames.size();

          set_frame_number(pte, frame_number);

          if (O) {
            printf("%d: ZERO     %4d\n", cnt_inst, frame_number);
            printf("%d: MAP  %4d%4d\n", cnt_inst, pi, frame_number);
          }

          ++cnt_zero;
          ++cnt_map;

          frames.push_back(frame_number);

          rev_frames[frame_number] = pi;

          
        } // have to replace a frame
        else {
          frame_number = algo->get_frame(pages, frames, rev_frames);

          unsigned int prev_pi = rev_frames[frame_number];

          unsigned int& prev_pte = pages[prev_pi];

          if (O) {
            printf("%d: UNMAP%4d%4d\n", cnt_inst, prev_pi, 
                frame_number);
          }
          ++cnt_unmap;

          clear_present_bit(prev_pte);
          clear_referenced_bit(prev_pte);

          if (get_modified_bit(prev_pte) == 1) {

            clear_modified_bit(prev_pte);
            set_pagedout_bit(prev_pte);

            if (O) {
              printf("%d: OUT  %4d%4d\n", cnt_inst, prev_pi, 
                  frame_number);
            }
            ++cnt_out;
          }

          if (get_pagedout_bit(pte) == 1) {

            if (O) {
              printf("%d: IN   %4d%4d\n", cnt_inst, pi, frame_number);
            }
            ++cnt_in;
          } 
          else {

            if (O) {
              printf("%d: ZERO     %4d\n", cnt_inst, frame_number);
            }
            ++cnt_zero;
          }

          if (O) {
            printf("%d: MAP  %4d%4d\n", cnt_inst, pi, frame_number);
          }
          ++cnt_map;

          set_frame_number(pte, frame_number);
          rev_frames[frame_number] = pi;
        }

        // set respective bit
        set_present_bit(pte);
        if (rw == 0) {
          set_referenced_bit(pte);
        } 
        else {
          set_referenced_bit(pte);
          set_modified_bit(pte);
        }

      } // if the corresponding entry is already in the frame table
      else {
        algo->update(frames, get_frame_number(pte));
        if (rw == 0) {
          set_referenced_bit(pte);
        } 
        else {
          set_referenced_bit(pte);
          set_modified_bit(pte);
        }
      }
      ++cnt_inst;

      // print virtual pages each cycle
      if (p) {
        for (int i = 0; i < pages.size(); ++i) {
          if (get_present_bit(pages[i]) == 1) {
            printf("%d:", i);
            if (get_referenced_bit(pages[i]) == 1) {
              printf("R");
            } 
            else {
              printf("-");
            }

            if (get_modified_bit(pages[i]) == 1) {
              printf("M");
            } 
            else {
              printf("-");
            }

            if (get_pagedout_bit(pages[i]) == 1) {
              printf("S ");
            } 
            else {
              printf("- ");
            }

          } 
          else {
            if (get_pagedout_bit(pages[i]) == 1) {
              printf("# ");
            } 
            else {
              printf("* ");
            }
          }
        }
        printf("\n");
      }

      // print phyical frames each cycle
      if (f) {
        for (int i = 0; i < rev_frames.size(); ++i) {
          if (rev_frames[i] == -1) {
            printf("* ");
          } 
          else {
            printf("%d ", rev_frames[i]);
          }
        }
        printf("\n");
      }
    } // end map_page_frame


    void print_summary() {
      if (P) {
        for (int i = 0; i < pages.size(); ++i) {
          if (get_present_bit(pages[i]) == 1) {
            printf("%d:", i);
            if (get_referenced_bit(pages[i]) == 1) {
              printf("R");
            } 
            else {
              printf("-");
            }

            if (get_modified_bit(pages[i]) == 1) {
              printf("M");
            } 
            else {
              printf("-");
            }

            if (get_pagedout_bit(pages[i]) == 1) {
              printf("S ");
            } 
            else {
              printf("- ");
            }
          } 
          else {
            if (get_pagedout_bit(pages[i]) == 1) {
              printf("# ");
            } 
            else {
              printf("* ");
            }
          }
        }
        printf("\n");
      }
      if (F) {
        for (int i = 0; i < rev_frames.size(); ++i) {
          if (rev_frames[i] == -1) {
            printf("* ");
          } 
          else {
            printf("%d ", rev_frames[i]);
          }
        }
        printf("\n");
      }
      if (S) {
        unsigned long long cost;

        cost = (long long)cnt_inst + 400 * (long long)(cnt_map + cnt_unmap) + 
          3000 * (long long)(cnt_in + cnt_out) + 150 * (long long)cnt_zero;

        printf("SUM %d U=%d M=%d I=%d O=%d Z=%d ===> %llu\n",
               cnt_inst, cnt_unmap, cnt_map, cnt_in,
               cnt_out, cnt_zero, cost);
      }
    }
};

void read_random_file(string s, vector<int>& rand_nums, int& count_random) {
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
    fprintf(stderr, "Unable to open random file\n");
  }
  fin.close();
}


void get_next_instruction(unsigned int& rw, unsigned int& pi, FILE** f) {
  FILE *infile = *f;
  char buf[1000];
  fgets(buf, 1000, infile);

  if (feof(infile) || buf[0] == '#') {
    rw = pi = -1;
  } 
  else {
    sscanf(buf, "%d%d", &rw, &pi);
    if (pi < 0 || pi > 63) {
      fprintf(stderr, "Virtual page index is out of range\n");
    }
  }
}

int main(int argc, char* argv[]) {

    int num_of_frames; // size of frame table
    bool O, P, F, S, p, f, a;
    Pager* algo; // page replacement algorithm used
    unsigned int rw, pi;
    int count_random;
    vector<int> rand_nums;

    FILE* infile = NULL;
    num_of_frames = 32;
    O = P = F = S = p = f = a = false;
    algo = new Pager_LRU();

    // parse optional arguments
    int c;
    int optlen;
    while ((c = getopt(argc, argv, "a:o:f:")) != -1) {
      switch (c) {
        // set algorithm 
        case 'a':
          switch (optarg[0]) {
            case 'N': algo = new Pager_NRU(); break;
            case 'l': algo = new Pager_LRU(); break;
            case 'r': algo = new Pager_Random(); break;
            case 'f': algo = new Pager_FIFO(); break;
            case 's': algo = new Pager_SecondChance(); break;
            case 'c': algo = new Pager_Clock_P(); break;
            case 'X': algo = new Pager_Clock_V(); break;
            case 'a': algo = new Pager_Aging_P(); break;
            case 'Y': algo = new Pager_Aging_V(); break;
            default: break;
          }
          break;
          // set options
        case 'o':
          optlen = strlen(optarg);
          for (int i = 0; i < optlen; ++i) {
            switch (optarg[i]) {
              case 'O': O = true; break;
              case 'P': P = true; break;
              case 'F': F = true; break;
              case 'S': S = true; break;
              case 'p': p = true; break;
              case 'f': f = true; break;
              case 'a': a = true; break;
            }
          }
          break;
          // set number of frames
        case 'f':
          sscanf(optarg, "%d", &num_of_frames);
          break;
      }
    }

    if (num_of_frames > 64) {
      fprintf(stderr, "Max. number of frames can't be greater than 64\n");
      abort();
    }

    infile = fopen(argv[argc - 2], "r");

    if (infile == NULL) {
      fprintf(stderr, "Unable to open input file\n");
      abort();
    }

    read_random_file(argv[argc - 1], rand_nums, count_random);
    algo->set_random(rand_nums, count_random);
    algo->set_num_of_frames(num_of_frames);

    VMM v = VMM(algo, num_of_frames, O, P, F, S, p, f, a);

    while(!feof(infile)) {
      get_next_instruction(rw, pi, &infile);

      // not a instruction line
      if (rw == -1 && pi == -1) continue;

      v.map_page_frame(rw, pi);
    }
    
    v.print_summary();

    if (infile != NULL) {
      fclose(infile);
    }
    return 0;
}
