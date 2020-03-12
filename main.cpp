#include <fstream>
#include <sstream>
#include <iomanip>
#include <time.h>
#include <cassert>
#include <algorithm>
#include <tuple>

#include "global.hpp"
#include "op.hpp"
#include "cnf.hpp"

using namespace std;


int main(int argc, char** argv) {
  string efilename = "e.txt";
  string ffilename = "f.txt";
  string gfilename = "g.txt";
  string cfilename = "_test.cnf";
  string rfilename = "_test.out";
  string dfilename = "_out.dot";
  string ofilename = "out" + to_string(time(NULL));
  string satcmd = "minisat -rnd-init " + cfilename + " " + rfilename;
  //string satcmd = "glucose " + cfilename + " " + rfilename;
  //string satcmd = "lingeling " + cfilename + " > " + rfilename;
  //string satcmd = "plingeling " + cfilename + " > " + rfilename;
  
  int ftransform = 0;
  int fmac = 0;
  int fextmem = 0;
  int finc = 0;
  int filp = 0;
  int ncycles = 0;
  int nregs = 1;
  int npipeline = 0;
  int nverbose = 0;

  // read options
  for(int i = 1; i < argc; i++) {
    if(argv[i][0] == '-') {
      switch(argv[i][1]) {
      case 'n':
	try {
	  ncycles = stoi(argv[++i]);
	}
	catch(...) {
	  show_error("-n must be followed by integer");
	}
	if(ncycles <= 0) {
	  show_error("the number of cycles must be more than 0");
	}
	break;
      case 'e':
	if(i+1 >= argc) {
	  show_error("-e must be followed by file name");
	}
	efilename = argv[++i];
	break;
      case 'f':
	if(i+1 >= argc) {
	  show_error("-f must be followed by file name");
	}
	ffilename = argv[++i];
	break;
      case 'g':
	if(i+1 >= argc) {
	  show_error("-g must be followed by file name");
	}
	gfilename = argv[++i];
	break;
      case 'c':
	ftransform ^= 1;
	break;
      case 'm':
	fmac ^= 1;
	break;
      case 'x':
	fextmem ^= 1;
	break;
      case 't':
	finc ^= 1;
	break;
      case 'i':
	filp ^= 1;
	break;
      case 'r':
	if(i+1 >= argc || argv[i+1][0] == '-') {
	  nregs = -1;
	  break;
	}
	try {
	  nregs = stoi(argv[++i]);
	}
	catch(...) {
	  show_error("-r should be followed by integer");
	}
	if(nregs == 0) {
	  show_error("the number of registers must not be 0");
	}
	break;
      case 's':
	try {
	  npipeline = stoi(argv[++i]);
	}
	catch(...) {
	  show_error("-s must be followed by integer");
	}
	break;
      case 'v':
	if(i+1 >= argc || argv[i+1][0] == '-') {
	  nverbose = 1;
	  break;
	}
	try {
	  nverbose = stoi(argv[++i]);
	}
	catch(...) {
	  show_error("-v should be followed by integer");
	}
	break;
      case 'h':
	cout << "usage : gen <options>" << endl;
	cout << "\t-h       : show this usage" << endl;
	cout << "\t-n <int> : the number of cycles [default = " << ncycles << "]" << endl;
	cout << "\t-e <str> : the name of environment file [default = \"" << efilename << "\"]" << endl;
	cout << "\t-f <str> : the name of formula file [default = \"" << ffilename << "\"]" << endl;
	cout << "\t-c       : toggle transforming dataflow [default = " << ftransform << "]" << endl;
	cout << "\t-m       : toggle using MAC operation [default = " << fmac << "]" << endl;
	cout << "\t-x       : toggle using external memory to store intermediate data [default = " << fextmem << "]" << endl;
	cout << "\t-t       : toggle incremental synthesis [default = " << finc << "]" << endl;
	cout << "\t-i       : toggle ilp [default = " << filp << "]" << endl;
	cout << "\t-r <int> : the number of registers in each PE (unspecified int is treated as no limit) [default = " << nregs << "]" << endl;
	cout << "\t-s <int> : the number of cycles for pipeline [default = " << npipeline << "]" << endl;
	cout << "\t-v <int> : toggle verbosing information [default = " << nverbose << "]" << endl;
	cout << "\t           \t0 : nothing" << endl;
	cout << "\t           \t1 : results" << endl;
	cout << "\t           \t2 : settings and results" << endl;
	return 0;
      default:
	show_error("invalid option " + string(argv[i]));
      }
    }
  }

  // read environment file
  ifstream efile(efilename);
  if(!efile) {
    show_error("cannot open environment file");
  }
  int nnodes = 0;
  map<string, int> node_name2id;
  vector<int> pe_nodes;
  vector<int> mem_nodes;
  vector<tuple<vector<int>, vector<int>, int> > coms;
  // external memory
  string extmem_name = "_extmem";
  node_name2id[extmem_name] = nnodes;
  mem_nodes.push_back(nnodes);
  nnodes++;
  string str;
  while(getline(efile, str)) {
    string s;
    stringstream ss(str);
    vector<string> vs;
    while(getline(ss, s, ' ')) {
      vs.push_back(s);
    }
    if(vs.empty()) {
      continue;
    }
    if(vs[0] == ".pe") {
      for(int i = 1; i < vs.size(); i++) {
	node_name2id[vs[i]] = nnodes;
	pe_nodes.push_back(nnodes);
	nnodes++;
      }
    }
    if(vs[0] == ".mem") {
      for(int i = 1; i < vs.size(); i++) {
	node_name2id[vs[i]] = nnodes;
	mem_nodes.push_back(nnodes);
	nnodes++;
      }
    }
    if(vs[0] == ".com") {
      while(getline(efile, str)) {
	vs.clear();
	stringstream ss2(str);
	while(getline(ss2, s, ' ')) {
	  vs.push_back(s);
	}
	if(vs.empty()) {
	  continue;
	}
	if(vs[0][0] == '.') {
	  break;
	}
	int i = 0;
	vector<int> senders;
	while(i < vs.size() && vs[i] != "->") {
	  if(!node_name2id.count(vs[i])) {
	    show_error("node " + vs[i] + " unspecified");
	  }
	  senders.push_back(node_name2id[vs[i]]);
	  i++;
	}
	if(i == vs.size()) {
	  show_error("there is an incomplete line in .com");
	}
	i++;
	vector<int> recipients;
	while(i < vs.size() && vs[i] != ":") {
	  if(!node_name2id.count(vs[i])) {
	    show_error("node " + vs[i] + " unspecified");
	  }
	  recipients.push_back(node_name2id[vs[i]]);
	  i++;
	}
	int band = -1;
	if(vs[i] == ":") {
	  i++;
	  if(i == vs.size()) {
	    show_error("there is an incomplete line in .com");
	  }
	  try {
	    band = stoi(vs[i]);
	  }
	  catch(...) {
	    show_error("bandwidth must be integer");
	  }
	}
	auto com = make_tuple(senders, recipients, band);
	coms.push_back(com);
      }
    }
  }
  efile.close();
  
  if(nverbose >= 2) {
    cout << "### environment information ###" << endl;
    cout << "id to node name :" << endl;
    for(auto i : node_name2id) {
      cout << i.second << " : " << i.first << endl;;
    }
    cout << "PEs :" << endl;
    for(int i : pe_nodes) {
      cout << i << ",";
    }
    cout << endl;
    cout << "Mems :" << endl;
    for(int i : mem_nodes) {
      cout << i << ",";
    }
    cout << endl;
    cout << "communication paths :" << endl;
    for(auto com : coms) {
      for(int i : get<0>(com)) {
	cout << i << " ";
      }
      cout << "-> ";
      for(int i : get<1>(com)) {
	cout << i << " ";
      }
      if(get<2>(com) > 0) {
	cout << ": " << get<2>(com);
      }
      cout << endl;
    }
  }

  // read formula file
  ifstream ffile(ffilename);
  if(!ffile) {
    show_error("cannot open formula file");
  }
  int ninputs = 0;
  map<string, opnode *> data_name2opnode;
  vector<string> datanames;
  vector<string> outputnames;
  vector<opnode *> outputs;
  while(getline(ffile, str)) {
    string s;
    stringstream ss(str);
    vector<string> vs;
    while(getline(ss, s, ' ')) {
      vs.push_back(s);
    }
    if(vs.empty()) {
      continue;
    }
    if(vs[0] == ".i") {
      for(int i = 1; i < vs.size(); i++) {
	datanames.push_back(vs[i]);
	opnode * p = new opnode;
	p->type = 0;
	p->id = ninputs++;
	data_name2opnode[vs[i]] = p;
      }
    }
    else if(vs[0] == ".o") {
      for(int i = 1; i < vs.size(); i++) {
	outputnames.push_back(vs[i]);
      }
    }
    else {
      int pos = 1;
      opnode * p = create_opnode(vs, pos, data_name2opnode);
      if(data_name2opnode.count(vs[0])) {
	show_error("data name in formula duplicated");
      }
      data_name2opnode[vs[0]] = p;
    }
  }
  ffile.close();
  for(auto s : outputnames) {
    opnode * p = data_name2opnode[s];
    if(!p) {
      show_error("output data " + s + " function unspecified");
    }
    outputs.push_back(p);
  }

  if(nverbose >= 2) {
    cout << "### formula information ###" << endl;
    for(auto p : outputs) {
      print_opnode(p, 0);
    }
  }

  // read option file
  ifstream gfile(gfilename);
  map<int, set<int> > assignments;
  set<int> sinputs;
  for(int i = 0; i < ninputs; i++) {
    sinputs.insert(i);
  }
  assignments[0] = sinputs;
  if(!gfile) {
    cout << "no option file" << endl;
  }
  else {
    while(getline(gfile, str)) {
      string s;
      stringstream ss(str);
      vector<string> vs;
      while(getline(ss, s, ' ')) {
	vs.push_back(s);
      }
      if(vs.empty()) {
	continue;
      }
      if(vs[0] == ".assign") {
	while(getline(gfile, str)) {
	  vs.clear();
	  stringstream ss2(str);
	  while(getline(ss2, s, ' ')) {
	    vs.push_back(s);
	  }
	  if(vs.empty()) {
	    continue;
	  }
	  if(vs[0][0] == '.') {
	    break;
	  }
	  if(!node_name2id.count(vs[0])) {
	    show_error("node " + vs[0] + " does not exist");
	  }
	  int id = node_name2id[vs[0]];
	  if(find(mem_nodes.begin(), mem_nodes.end(), id) == mem_nodes.end()) {
	    show_error("node " + vs[0] + " is not Mem");
	  }
	  sinputs.clear();
	  for(int i = 1; i < vs.size(); i++) {
	    opnode * p = data_name2opnode[vs[i]];
	    if(!p) {
	      show_error("unspecified data " + vs[i] + " appears in option");
	    }
	    if(p->id == -1) {
	      show_error("data " + vs[i] + " is not input");
	    }
	    sinputs.insert(p->id);
	  }
	  assignments[id] = sinputs;
	}
      }
    }
    
    if(nverbose >= 2) {
      cout << "### option information ###" << endl;
      cout << "assignments :" << endl;
      for(auto assignment : assignments) {
	cout << assignment.first << " <- ";
	for(auto id : assignment.second) {
	  cout << id << ",";
	}
	cout << endl;
      }
    }
  }

  // apply compression
  if(ftransform) {
    for(auto p : outputs) {
      compress_opnode(p);
      // notice : memory leaks here
    }
    if(nverbose >= 2) {
      cout << "### formula after compression ###" << endl;
      for(auto p : outputs) {
	print_opnode(p, 0);
      }
    }
  }

  // generate operand list
  int ndata = ninputs;
  vector<int> optypes(ndata);
  vector<set<set<int> > > operands(ndata);
  map<pair<int, multiset<int> >, int> unique;
  set<int> output_ids;
  for(auto p : outputs) {
    gen_operands(p, ndata, optypes, operands, unique, datanames);
    output_ids.insert(p->id);
  }
  if(fmac) {
    support_MAC(optypes, operands);
  }
  assert(ndata == operands.size());

  if(nverbose >= 2) {
    cout << "### operand list ###" << endl;
    int d = 0;
    for(auto a : operands) {
      cout << "data " << d << " :" << endl;
      for(auto b : a) {
	for(auto c : b) {
	  cout << "\t" << setw(3) << c << ",";
	}
	cout << endl;
      }
      d++;
    }
  }
  
  // instanciate problem generator
  Cnf cnf = Cnf(pe_nodes, mem_nodes, coms, ninputs, output_ids, assignments, operands);
  if(filp) {
    cnf.filp = 1;
    cfilename = "_test.lp";
  }
  
  if(finc && ncycles < 1) {
    ncycles = 1;
  }
  
  if(ncycles < 1) {
    cout << "your files are valid" << endl;
    cout << "to run synthesis, please specify the number of cycles by using option -n" << endl;
    return 0;
  }

  while(1) {
    cout << "ncycles : " << ncycles << endl;
    cnf.gen_cnf(ncycles, nregs, fextmem, npipeline, cfilename);
    if(filp) {
      return 0;
    }
    system(satcmd.c_str());
    int r = 0;
    ifstream rfile(rfilename);
    if(!rfile) {
      show_error("cannot open result file");
    }
    while(getline(rfile, str)) {
      string s;
      stringstream ss(str);
      vector<string> vs;
      getline(ss, s, ' ');
      if(s == "SAT" || s == "1" || s == "-1") {
	r = 1;
	break;
      }
      else if(s == "UNSAT") {
	break;
      }
      else if(s == "s") {
	getline(ss, s, ' ');
	if(s == "SATISFIABLE") {
	  r = 1;
	  break;
	}
	else if(s == "UNSATISFIABLE") {
	  break;
	}
      }
    }
    // show results
    if(r) {
      std::cout << "Solution found" << std::endl;
      break;
    }
    else {
      std::cout << "No solution" << std::endl;
    }
    if(!finc) {
      return 0;
    }
    ncycles++;
  }

  cnf.gen_image(rfilename);

  //  cnf.reduce_image();
  
  if(nverbose) {
    cout << "### results ###" << endl;
    for(int k = 0; k < cnf.image.size(); k++) {
      cout << "cycle " << k << " :" << endl;
      for(int j = 0; j < cnf.image[0].size(); j++) {
	cout << "\tnode " << j << " :";
	for(int i : cnf.image[k][j]) {
	  cout << " " << i << "(" << datanames[i] << ")";
	}
	cout << endl;
      }
    }
  }

  return 0;
}
