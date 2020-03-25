#include <fstream>
#include <sstream>
#include <iomanip>
#include <time.h>
#include <cassert>
#include <algorithm>
#include <tuple>

#include "global.hpp"
#include "dfg.hpp"
#include "cnf.hpp"

using namespace std;


int main(int argc, char** argv) {
  string efilename = "e.txt";
  string ffilename = "f.txt";
  string gfilename = "g.txt";
  string cfilename = "_test.cnf";
  string rfilename = "_test.out";
  string satcmd = "timeout 1d minisat " + cfilename + " " + rfilename;
  //string satcmd = "glucose " + cfilename + " " + rfilename;
  //string satcmd = "lingeling " + cfilename + " > " + rfilename;
  //string satcmd = "plingeling " + cfilename + " > " + rfilename;
  
  int ncycles = 0;
  int nregs = 2;
  int nprocs = 1;
  
  int fextmem = 0;
  int ftransform = 0;
  int npipeline = 0;

  int nencode = 3;
  int finc = 0;
  int filp = 0;
  int nverbose = 0;

  // read options
  for(int i = 1; i < argc; i++) {
    if(argv[i][0] == '-') {
      switch(argv[i][1]) {
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
      case 'p':
	if(i+1 >= argc || argv[i+1][0] == '-') {
	  nprocs = -1;
	  break;
	}
	try {
	  nprocs = stoi(argv[++i]);
	}
	catch(...) {
	  show_error("-p should be followed by integer");
	}
	if(nprocs == 0) {
	  show_error("the number of processors must not be 0");
	}
	break;
      case 'x':
	fextmem ^= 1;
	break;
      case 'c':
	ftransform ^= 1;
	break;
      case 't':
	try {
	  npipeline = stoi(argv[++i]);
	}
	catch(...) {
	  show_error("-t must be followed by integer");
	}
	if(npipeline <= 0) {
	  show_error("the number of contexts must be more than 0");
	}
	break;
      case 'a':
	finc ^= 1;
	break;
      case 'l':
	try {
	  nencode = stoi(argv[++i]);
	}
	catch(...) {
	  show_error("-l must be followed by integer");
	}
	if(nencode < 0 || nencode > 4) {
	  show_error("the number of cycles must be between 0 and 4");
	}
	break;
      case 'i':
	filp ^= 1;
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
	cout << "\t-e <str> : the name of environment file [default = \"" << efilename << "\"]" << endl;
	cout << "\t-f <str> : the name of formula file [default = \"" << ffilename << "\"]" << endl;
	cout << "\t-g <str> : the name of option file [default = \"" << gfilename << "\"]" << endl;
	cout << "\t-n <int> : the number of cycles [default = " << ncycles << "]" << endl;
	cout << "\t-r <int> : the number of registers in each PE (just -r means no limit) [default = " << nregs << "]" << endl;
	cout << "\t-u <int> : the number of processors in each PE (just -u means no limit) [default = " << nprocs << "]" << endl;
	cout << "\t-x       : toggle enabling external memory to store intermediate values [default = " << fextmem << "]" << endl;
	cout << "\t-c       : toggle transforming dataflow [default = " << ftransform << "]" << endl;
	cout << "\t-t <int> : the number of contexts for pipeline (0 means no pipelining) [default = " << npipeline << "]" << endl;
	cout << "\t-a       : toggle incremental synthesis [default = " << finc << "]" << endl;
	cout << "\t-l <int> : the type of at most one encoding [default = " << nencode << "]" << endl;
	cout << "\t           \t0 : naive" << endl;
	cout << "\t           \t1 : commander" << endl;
	cout << "\t           \t2 : binary" << endl;
	cout << "\t           \t3 : bimander half" << endl;
	cout << "\t           \t4 : bimander root" << endl;	
	cout << "\t-i       : toggle using ILP solver instead of SAT solver [default = " << filp << "]" << endl;
	cout << "\t-v <int> : the level of verbosing information [default = " << nverbose << "]" << endl;
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
	  if(band <= 0) {
	    show_error("bandwidth must be more than 0");
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
  Dfg dfg;
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
	dfg.create_input(vs[i]);
      }
    }
    if(vs[0] == ".o") {
      for(int i = 1; i < vs.size(); i++) {
	dfg.outputnames.push_back(vs[i]);
      }
    }
    if(vs[0] == ".f") {
      while(getline(ffile, str)) {
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
	if(vs.size() < 2) {
	  show_error("operator must be followed by the number of operands");
	}
	int n;
	try {
	  n = stoi(vs[1]);
	}
	catch(...) {
	  show_error("operator must be followed by the number of operands");
	}
	n = dfg.add_operator(vs[0], n);
	for(int i = 2; i < vs.size(); i++) {
	  if(vs[i] == "c") {
	    dfg.make_commutative(n);
	  }
	  if(vs[i] == "a") {
	    dfg.make_associative(n);
	  }
	}
      }
    }
    if(vs[0] == ".m") {
      while(getline(ffile, str)) {
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
	dfg.add_multiope(vs);
      }
    }
    if(vs[0] == ".n") {
      while(getline(ffile, str)) {
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
	dfg.create_opnode(vs);
      }
    }
  }
  ffile.close();

  if(nverbose >= 2) {
    cout << "### formula information ###" << endl;
    dfg.print();
  }

  // read option file
  ifstream gfile(gfilename);
  map<int, set<int> > assignments;
  set<int> sinputs;
  for(int i = 0; i < dfg.ninputs; i++) {
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
	    sinputs.insert(dfg.input_id(vs[i]));
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
    dfg.compress();
    if(nverbose >= 2) {
      cout << "### formula after compression ###" << endl;
      dfg.print();
    }
  }

  // generate operand list
  dfg.gen_operands();
  assert(dfg.ndata == dfg.operands.size());

  if(nverbose >= 2) {
    cout << "### operand list ###" << endl;
    int d = 0;
    for(auto a : dfg.operands) {
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
  Cnf cnf = Cnf(pe_nodes, mem_nodes, coms, dfg.ninputs, dfg.output_ids(), assignments, dfg.operands);
  cnf.nencode = nencode;
  if(dfg.fmulti) {
    cnf.fmultiop = 1;
  }
  if(filp) {
    cnf.filp = 1;
    cfilename = "_test.lp";
    rfilename = "_test.sol";
    satcmd = "timeout 1d cplex -c \"read " + cfilename + "\" \"set emphasis mip 1\" \"set threads 1\" \"optimize\" \"write " +  rfilename + "\"";
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
    cout << "ndata : " << dfg.ndata << endl;
    cnf.gen_cnf(ncycles, nregs, nprocs, fextmem, npipeline, cfilename);
    int r = system(satcmd.c_str());
    r = r >> 8;
    if(r == 124) {
      cout << "Timeout" << endl;
      return 0;
    }
    ifstream rfile(rfilename);
    if(filp) {
      if(!rfile) {
	std::cout << "No solution" << std::endl;
	if(!finc) {
	  return 0;
	}
	ncycles++;
	continue;
      }
      else {
	std::cout << "Solution found" << std::endl;
	break;
      }
    }
    if(!rfile) {
      show_error("cannot open result file");
    }
    r = 0;
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

  if(filp) {
    return 0;
    // TODO read results
  }
    
  cnf.gen_image(rfilename);

  //  cnf.reduce_image();
  
  if(nverbose) {
    map<int, string> node_id2name;
    for(auto elem : node_name2id) {
      node_id2name[elem.second] = elem.first;
    }
    for(int j = 0; j < coms.size(); j++) {
      auto com = coms[j];
      string name;
      for(int i : get<0>(com)) {
	name += node_id2name[i] + " ";
      }
      name += "-> ";
      for(int i : get<1>(com)) {
	name += node_id2name[i] + " ";
      }
      name.pop_back();
      node_id2name[nnodes+j] = name;
    }
    cout << "### results ###" << endl;
    for(int k = 0; k < cnf.image.size(); k++) {
      cout << "cycle " << k << " :" << endl;
      for(int j = 0; j < cnf.image[0].size(); j++) {
	cout << "\t" << node_id2name[j] << " :";
	for(int i : cnf.image[k][j]) {
	  cout << " " << i << "#" << dfg.datanames[i];
	}
	cout << endl;
      }
    }
  }

  return 0;
}
