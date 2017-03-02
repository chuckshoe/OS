#include <iostream>
#include <fstream>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <cctype>
#include <utility>
#include <map>

using namespace std;

const string ERROR_SYMBOL = "__ERROR__";
const string EMPTY_SYMBOL = "__EMPTY__";
const int MAX_INSTRUCTION = 512;

typedef struct SYMBOL {
  bool notused;
  string name;
  int val;
  SYMBOL() : notused(true) {}
} Symbol;

typedef struct TOKEN {
  string s;
  int line;
  int offset;
} Token;

vector<int> module_address;
vector<vector<Symbol> > def_symbols;
vector< pair<string,bool> > use_symbols;

map<string, int> symbol_count_table;

string in; // stores current line
int line_count = 0;
int len = -1; // -1 indicates no active line
int curr_pos; // current position in line
bool empty; 
int lastLen = 1;
int lastOffset = 0;
int num_of_modules = 0;
int instruction_count = 0;
int stream_length;
bool module_start;
ifstream fin;

void parseerror(int linenum, int lineoffset, int errcode) { 
  static string errstr[] = { 
    "NUM_EXPECTED",
    "SYM_EXPECTED",
    "ADDR_EXPECTED",
    "SYM_TOLONG",
    "TO_MANY_DEF_IN_MODULE",
    "TO_MANY_USE_IN_MODULE", 
    "TO_MANY_INSTR"
  }; 
  printf("Parse Error line %d offset %d: %s\n", linenum, lineoffset, 
      errstr[errcode].c_str()); 
} 

int check_number(string in) {
  int n = 0;
  int l = in.size();

  for (int i = 0; i < l; ++i) {
    if (!isdigit(in[i])) {
      n = -1;
      break;
    } else {
      n = n * 10 + in[i] - '0';
    }
  }
  return n;
}

string check_symbol(string in) {
  int len = in.length();

  if (!isalpha(in[0])) {
    return ERROR_SYMBOL;
  }
  for (int i = 1; i < len; ++i) {
    if (!isalnum(in[i])) {
      return ERROR_SYMBOL;
    }
  }
  return in;
}

string check_addressing(string in) {
  if (in.length() > 1) {
    return ERROR_SYMBOL;
  } else {
    if (in[0] != 'I' && in[0] != 'A' &&
        in[0] != 'R' && in[0] != 'E') {
      return ERROR_SYMBOL;
    } else {
      return in;
    }
  }
}

bool check_opcode(string addr) {
  int  len = addr.length();
  bool ret = true;
  for (int i = 0; i < len; ++i) {
    if (!(addr[i] >= '0' && addr[i] <= '9')) {
      ret = false;
      break;
    }
  }
  return ret && (len <= 4);
}

Token get_token() {
  Token t;
  // case 1: no active line
  if (len == -1) { 
    if (getline(fin, in) ) { // check if u get a line
      ++line_count;
      empty = true; // assuming we have empty line, i.e. all whitespace
      curr_pos = 0;
      len = in.size();
      for (int i = curr_pos; i<len ; i++) {
        if (!isspace(in[i])) {
          empty = false; //token there in this line,not empty line
          int j;
          for (j = i; j<len; j++) {
            if(isspace(in[j]))
              break;
          }
          t.s = in.substr(i, j-i);
          t.line = line_count;
          t.offset = i + 1; //+1 coz offset count starts from 1
          lastOffset = t.offset + t.s.length();
          curr_pos = j;
          break;
        }
      }
      if (!empty) { // exited loop with a token
        if (curr_pos == len) {
          lastLen = len;
          len = -1;
        }
        return t;
      }
      else { // didn't find token in this line
        lastLen = len;
        len = -1;
        return get_token();
      }
    }
    // didn't get a line from file
    t.s = EMPTY_SYMBOL;
    t.line = line_count;
    t.offset = empty ? lastLen > 0 ? lastLen : 1 : lastOffset;
    return t;
  }
  
  // case 2: we have active line
  int i;
  int flag = 0;      
  for (int i = curr_pos; i<len ; i++) {
    if (!isspace(in[i])) {
      flag = 1; //token there in this line,not empty line
      int j;
      for (j = i; j<len; j++) {
        if(isspace(in[j]))
          break;
      }
      t.s = in.substr(i, j-i);
      t.line = line_count;
      t.offset = i + 1; //+1 coz offset count starts from 1
      lastOffset = t.offset + t.s.length();
      curr_pos = j;
      break;
    }
  }
  
  if (flag) { // exited loop with a token
    if (curr_pos == len) {
      lastLen = len;
      len = -1;
    }
    return t;
  }
  else { // didn't find token in this line
    lastLen = len;
    len = -1;
    return get_token();
  }
}

int read_int(Token *t) {
  *t = get_token();
  if ((*t).s.compare(EMPTY_SYMBOL) == 0) return -2;
  return check_number((*t).s);
}

string read_symbol(Token *t) {
  *t = get_token();
  if ((*t).s.compare(EMPTY_SYMBOL) == 0) (*t).s = ERROR_SYMBOL;
  return check_symbol((*t).s);
}
void read_definition(vector<Symbol> *module_def_symbols ) {
  string str;
  int val;
  Token t;
  Symbol symbol;

  str = read_symbol(&t);
  if (str == ERROR_SYMBOL) {
      parseerror(t.line, t.offset, 1);
      exit(-1);
  } 
  else if (str.length() > 16) {
      parseerror(t.line, t.offset, 3);
      exit(-1);
  }
  val = read_int(&t);
  if ((val == -1) || (val == -2)) {
    parseerror(t.line, t.offset, 0);
    exit(-1);
  }
  
  if (symbol_count_table.count(str) != 0 ) {// symbol already defined
    symbol_count_table[str] += 1;
  }  
  else { // new symbol
    symbol_count_table[str] = 1;
    symbol.name = str;
    symbol.val = val;
    (*module_def_symbols).push_back(symbol);
  }
}

bool read_definition_list() {
  int defcount;
  int i;
  Token t;
  vector<Symbol> module_def_symbols;

  defcount = read_int(&t);
  if (module_start && (defcount == -2)) return false;
  if (defcount == -1) {
    parseerror(t.line, t.offset, 0);
    exit(-1);
  }
  else if (defcount >16) {
    parseerror(t.line, t.offset, 4);
    exit(-1);
  }
  module_start = false;
  if (defcount == 0) {
    Symbol symbol;
    symbol.name = ERROR_SYMBOL;
    symbol.val = -1;
    module_def_symbols.push_back(symbol); 
  }
  else {
    for (i=1; i<=defcount; i++) {
      read_definition(&module_def_symbols);
    }
  }
  def_symbols.push_back(module_def_symbols);
  return true;
}

void read_use_list() {
  int usecount;
  string str;
  Token t;

  usecount = read_int(&t);
  if ((usecount == -1) || (usecount == -2)) {
    parseerror(t.line, t.offset, 0);
    exit(-1);
  }
  else if (usecount > 16) {
    parseerror(t.line, t.offset, 5);
    exit(-1);
  }
  if (usecount == 0) {
    // do nothing
  }
  else {
    for (int i = 1; i <= usecount; i++) {
      str = read_symbol(&t);
      if (str == ERROR_SYMBOL) {
          parseerror(t.line, t.offset, 1);
          exit(-1);
      } 
      else if (str.length() > 16) {
          parseerror(t.line, t.offset, 3);
          exit(-1);
      }
    } 
  }
}

void read_instruction() {
  string addr;
  int val;
  Token t;

  t = get_token();
  if (t.s.compare(EMPTY_SYMBOL) == 0) t.s = ERROR_SYMBOL;
  addr = check_addressing(t.s);
  if (addr == ERROR_SYMBOL) {
      parseerror(t.line, t.offset, 2);
      exit(-1);
  } 
  
  val = read_int(&t);
  if ((val== -1) ||(val== -2) ) {
    parseerror(t.line, t.offset, 0);
    exit(-1);
  }
}

void update_symbol_addresses(int codecount) {
  int i;
  Symbol s;
  for (i = 0; i < def_symbols[num_of_modules-1].size(); i++) {
    s = def_symbols[num_of_modules-1][i];
    if ( (s.val == -1) && (s.name.compare(ERROR_SYMBOL) == 0) ) {
      break;
    }
    else {
      int base = 1 + module_address[num_of_modules - 2];
      s.val += base;
      def_symbols[num_of_modules-1][i].val = s.val;
    }
  }
}

void read_instruction_list() {
  int codecount;
  Token t;

  codecount = read_int(&t);
  if ((codecount == -1) || (codecount == -2)) {
    parseerror(t.line, t.offset, 0);
    exit(-1);
  }
  else if (codecount + instruction_count > MAX_INSTRUCTION) {
    parseerror(t.line, t.offset, 6);
    exit(-1);
  }

  for (int i = 0; i < codecount; i++) {
    read_instruction();
    instruction_count ++;
  }

  if (num_of_modules == 1) {
    module_address.push_back(codecount-1);
  }
  else {
    module_address.push_back(instruction_count-1);
  }

  if(num_of_modules > 1) 
    update_symbol_addresses(codecount);
}


int create_module(){
  ++num_of_modules;
  module_start = true;
  if (read_definition_list()) {
    read_use_list();
    read_instruction_list();
    return 1;
  }
  return 0;
}

void print_warnings() {
  int i;
  int j;
  Symbol s;
  for (i = 0; i < def_symbols.size(); i++) {
    for (j = 0; j < def_symbols[i].size(); j++) {
      s = def_symbols[i][j];
      if ( (s.val == -1) && (s.name.compare(ERROR_SYMBOL) == 0) ) {
        // do nothing
      }
      else {
        int offset;
        if (i == 0) {
          offset = s.val;
        }
        else {
          offset = s.val - (1 + module_address[i-1]);
        }
        if (s.val > module_address[i]) {
          int max;
          int addr;
          if (i == 0 ) {
            max = module_address[i] ;
            addr = 0;
          }
          else {
            max = module_address[i] - module_address[i - 1];
            addr = module_address[i-1] + 1;
          }
          cout << "Warning: Module " << i + 1 << ": " <<s.name << " to big " ;
          cout<< offset << " (max=" <<max ;
          cout<<") assume zero relative"<<endl;
          def_symbols[i][j].val = addr;
        }
      }
    }
  }
}

void print_symbol_table () {

  int i;
  int j;
  Symbol s;
  cout << "Symbol Table" << endl;
  for (i = 0; i < def_symbols.size(); i++) {
    for (j = 0; j < def_symbols[i].size(); j++) {
      s = def_symbols[i][j];
      if ( (s.val == -1) && (s.name.compare(ERROR_SYMBOL) == 0) ) {
        // do nothing
      }
      else {
        cout<<s.name<<"="<<s.val;
        if (symbol_count_table[s.name]>1){
          cout<<" Error: This variable is multiple times ";
          cout<<"defined; first value used"<<endl; 
        }
        else {
          cout<<endl;
        } 
      }
    }
  }
}

void first_pass() {
  while (fin.tellg() <= (stream_length-1)) {
    if( !create_module() ) break;
  }
  print_warnings();
  print_symbol_table();
  cout<<endl;  
}

void read_definition_second() {
  string str;
  int val;
  Token t;

  str = read_symbol(&t);

  val = read_int(&t);
}
bool read_definition_list_second() {
  int defcount;
  int i;
  Token t;

  defcount = read_int(&t);
  if (module_start && (defcount == -2)) return false;

  if (defcount == 0) {
    //do nothing;
  }
  else {
    for (i=1; i<=defcount; i++) {
      read_definition_second();
    }
  }
  return true;
}
void update_sym_use_status(string str) {
  int i;
  int j;
  Symbol s;
  for (i = 0; i < def_symbols.size(); i++) {
    for (j = 0;j < def_symbols[i].size() ; j++) {
      s = def_symbols[i][j];
      if (s.name.compare(str) == 0) 
        def_symbols[i][j].notused = false;  
    }
  }
}
void read_use_list_second() {
  int usecount;
  string str;
  Token t;

  usecount = read_int(&t);
  if (usecount == 0) {
    // do nothing
  }
  else {
    for (int i = 1; i <= usecount; i++) {
      str = read_symbol(&t);

      use_symbols.push_back(make_pair(str,false));
      update_sym_use_status(str);
    } 
  }
}

int get_symbol_address(string str) {
  int i;
  int j;
  Symbol s;
  for (i = 0; i < def_symbols.size(); i++) {
    for (j = 0;j < def_symbols[i].size() ; j++) {
      s = def_symbols[i][j];
      if (s.name.compare(str) == 0) {
        return def_symbols[i][j].val;        
      }  
    }
  }
  return -1;
}

void read_instruction_list_second() {
  int codecount;
  int base;
  Token t1;
  Token t2;
  string addr_type;
  int val;
  bool e;
  string addr;

  if (num_of_modules == 1) {
    base = 0;
  }
  else {
    base = 1 + module_address[num_of_modules - 2];
  }
  codecount = read_int(&t1);

  for (int k = 0; k < codecount; k++) {
    t1 = get_token();

    addr_type = check_addressing(t1.s);
    val = read_int(&t2);

    if (addr_type.compare("I") == 0) {
      e = check_opcode(t2.s);
      if (e) {
        printf("%03d: %04d\n",base+k,val);
      }
      else {
        printf("%03d: 9999 Error: Illegal immediate value; ",base+k);
        printf("treated as 9999\n");
      } 
    }
    else if (addr_type.compare("A") == 0) {
      e = check_opcode(t2.s);
      addr = t2.s;
      if(!e) {
        printf("%03d: 9999 Error: Illegal opcode; treated as 9999\n",base+k);
      }
      else {
        while (addr.size() < 4) {
          addr = "0" + addr;
        }
        int n = 0;
        for (int i = 1; i < addr.size(); ++i) {
          n = n * 10 + addr[i] - '0';
        }
        if (n >= MAX_INSTRUCTION) {
          printf("%03d: %c000 Error: Absolute address exceeds ",base+k,addr[0]);
          printf("machine size; zero used\n");
        }
        else {
          printf("%03d: %c%03d\n",base+k,addr[0],n);
        }
      }
    }
    else if (addr_type.compare("R") == 0) {
      e = check_opcode(t2.s);
      addr = t2.s;
      if(!e) {
        printf("%03d: 9999 Error: Illegal opcode; treated as 9999\n",base+k);
      }
      else {
        while (addr.size() < 4) {
          addr = "0" + addr;
        }
        int n = 0;
        for (int i = 1; i < addr.size(); ++i) {
          n = n * 10 + addr[i] - '0';
        }
        if (n + base > module_address[num_of_modules-1] ){
          printf("%03d: %c%03d Error: Relative address ",base+k,addr[0],base);
          printf("exceeds module size; zero used\n");
        }
        else {
          printf("%03d: %c%03d\n",base+k,addr[0],base+n);
        }
      }   
    }
    else if (addr_type.compare("E") == 0) {
      e = check_opcode(t2.s);
      addr = t2.s;
      if(!e) {
        printf("%03d: 9999 Error: Illegal opcode; treated as 9999\n",base+k);
      }
      else {
        while (addr.size() < 4) {
          addr = "0" + addr;
        }
        int n = 0;
        for (int i = 1; i < addr.size(); ++i) {
          n = n * 10 + addr[i] - '0';
        }
        if (n > use_symbols.size()-1) {
          printf("%03d: %04d Error: External address exceeds ",base+k,val);
          printf("length of uselist; treated as immediate\n");
        }
        else {
          string str = use_symbols[n].first;
          int val = get_symbol_address(str);
          use_symbols[n].second = true;
          if (val == -1) {
            //uselist symbol not defined , zero used
            printf("%03d: %c000 Error: %s is ",base+k,addr[0],str.c_str());
            printf("not defined; zero used\n");
          }
          else {
            //normal case
            printf("%03d: %c%03d\n",base+k,addr[0],val);
          }
        }
      }
    }
  }

  if (!use_symbols.empty()) {
    int i;
    for (i=0; i<use_symbols.size(); i++){
      if (!use_symbols[i].second) {
      printf ("Warning: Module %d: %s ",num_of_modules,
          use_symbols[i].first.c_str());
      printf ("appeared in the uselist but was not actually used\n");
      }
    }
  }
}

void print_warnings_second() {
  int i;
  int j;
  Symbol s;
  for (i = 0; i < def_symbols.size(); i++) {
    for (j = 0;j < def_symbols[i].size() ; j++) {
      s = def_symbols[i][j];
      if (s.notused && (s.name.compare(ERROR_SYMBOL)!=0)) {
        cout<<"Warning: Module "<<i+1<<": "<<s.name;
        cout<<" was defined but never used"<<endl;      
      }  
    }
  }
}

int create_module_second(){
  ++num_of_modules;
  module_start = true;
  if (read_definition_list_second()) {
    read_use_list_second();
    read_instruction_list_second();
    use_symbols.clear();
    return 1;
  }
  use_symbols.clear();
  return 0;
}
void second_pass() {
  num_of_modules = 0;
  cout<<"Memory Map"<<endl;
  while (cin.tellg() <= (stream_length-1)) {
    if( !create_module_second() ) break;
  }
  cout<<endl;
  print_warnings_second();
}

int main(int argc, char* argv[]) {
  //ifstream fin;
  if (argc == 1) {
    cout << "Please enter the file name." << endl;
    return -1;
  } 
  else {
    fin.open(argv[1]);
    if (!fin.is_open()) {
      cout << "Failed to open the file." << endl;
      return -1;
    }
  }

  fin.seekg(0, fin.end);
  stream_length = fin.tellg();
  fin.seekg(0, fin.beg);

  first_pass() ;
  
  fin.close();
  fin.open(argv[1]);
  second_pass() ;
  
  return 0;
}
