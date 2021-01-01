#include <fstream>
#include <sstream>
#include <cstdio>
#include <chrono>

#include "global.hpp"
#include "graph.hpp"
#include "dfg.hpp"
#include "cnf.hpp"

using namespace std;

int main(int argc, char** argv) {
  bool fout = 0;
  
  string efilename = "e.txt";
  string ffilename = "f.txt";
  string gfilename = "g.txt";
  string pfilename = "_test.cnf";
  string rfilename = "_test.out";
  string pfilename_ilp = "_test.lp";
  string rfilename_ilp = "_test.sol";
  string dfilename = "_test.dot";
  string ifilename_base = "_image";
  string ifilename_type = "png";

  vector<string> solver_cmds = {"minisat " + pfilename + " " + rfilename,
				"glucose " + pfilename + " " + rfilename,
				"lingeling " + pfilename + " | tee " + rfilename,
				"plingeling " + pfilename + " | tee " + rfilename};

  string solver_cmd_ilp = "cplex -c \"read " + pfilename_ilp + "\" \"set emphasis mip 1\" \"set threads 1\" \"optimize\" \"write " +  rfilename_ilp + "\"";

  string dot_cmd = "dot -T" + ifilename_type + " " + dfilename;

  int ncycles = 0;
  int nregs = 2;
  int nprocs = 1;
  int ncontexts = 0;
  
  bool fextmem = 0;
  bool ftransform = 0;
  bool fmultiopr = 1;

  bool filp = 0;
  bool freduce = 1;
  bool finc = 0;
  string timeout = "1d";
  
  int nsolver = 0;
  int nencode = 3;
  
  int nverbose = 0;
  bool fname = 1;

  // read command
  for(int i = 1; i < argc; i++) {
    if(argv[i][0] != '-' || argv[i][1] == '\0') {
      show_error("invalid option", argv[i]);
    }
    int i_ = i;
    for(int j = 1; argv[i_][j] != '\0'; j++) {
      if(i != i_) {
	show_error("invalid option", argv[i_]);
      }
      switch(argv[i_][j]) {
      case 'o':
	fout ^= 1;
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
      case 'n':
	try {
	  ncycles = str2int(argv[++i]);
	}
	catch(...) {
	  show_error("-n must be followed by integer");
	}
	if(ncycles < 0) {
	  show_error("the number of cycles must not be negative");
	}
	break;
      case 'r':
	try {
	  nregs = str2int(argv[++i]);
	}
	catch(...) {
	  show_error("-r must be followed by integer");
	}
	if(nregs < 0) {
	  show_error("the number of registers must not be negative");
	}
	break;
      case 'p':
	try {
	  nprocs = str2int(argv[++i]);
	}
	catch(...) {
	  show_error("-p must be followed by integer");
	}
	if(nprocs < 0) {
	  show_error("the number of processors must not be negative");
	}
	break;
      case 't':
	try {
	  ncontexts = str2int(argv[++i]);
	}
	catch(...) {
	  show_error("-t must be followed by integer");
	}
	if(ncontexts < 0) {
	  show_error("the number of contexts must not be negative");
	}
	break;
      case 'x':
	fextmem ^= 1;
	break;
      case 'c':
	ftransform ^= 1;
	break;
      case 'm':
	fmultiopr ^= 1;
	break;
      case 'i':
	filp ^= 1;
	break;
      case 'y':
	freduce ^= 1;
	break;
      case 'a':
	finc ^= 1;
	break;
      case 'z':
	fname ^= 1;
	break;
      case 'l':
	try {
	  if(i+1 >= argc) {
	    throw 0;
	  }
	  timeout = argv[++i];
	  char c = timeout.back();
	  int n;
	  if(c == 's' || c == 'm' || c == 'h' || c == 'd') {
	    n = str2int(timeout.substr(0, timeout.size()-1));
	  }
	  else {
	    n = str2int(timeout);
	  }
	  if(n == 0) {
	    timeout = "0";
	  }
	  if(n < 0) {
	    throw 0;
	  }
	}
	catch(...) {
	  show_error("-l must be followed by timeout duration");
	}
	break;
      case 'd':
	try {
	  nencode = str2int(argv[++i]);
	}
	catch(...) {
	  show_error("-d must be followed by integer");
	}
	if(nencode < 0 || nencode > 4) {
	  show_error("the encoding must be between 0 and 4");
	}
	break;
      case 's':
	try {
	  nsolver = str2int(argv[++i]);
	}
	catch(...) {
	  show_error("-s must be followed by integer");
	}
	if(nsolver < 0 || nsolver >= (int)solver_cmds.size()) {
	  show_error("SAT solver must be between 0 and ", to_string(solver_cmds.size()));
	}
	break;
      case 'v':
	try {
	  nverbose = str2int(argv[++i]);
	}
	catch(...) {
	  show_error("-v must be followed by integer");
	}
	if(nverbose < 0) {
	  show_error("the verbosing level must be between 0 and 3");
	}
	break;
      case 'h':
	cout << "usage : dfgmap <options>" << endl;
	cout << "\t-h       : show this usage" << endl;
	cout << "\t-o       : toggle generating output image files [default = " << fout << "]" << endl;
	cout << "\t-e <str> : the name of file for array processor [default = \"" << efilename << "\"]" << endl;
	cout << "\t-f <str> : the name of file for data-flow [default = \"" << ffilename << "\"]" << endl;
	cout << "\t-g <str> : the name of file for synthesis option (ignored if not exist) [default = \"" << gfilename << "\"]" << endl;
	cout << "\t-n <int> : the number of cycles [default = " << ncycles << "]" << endl;
	cout << "\t-r <int> : the number of registers in each PE (0 means no limit) [default = " << nregs << "]" << endl;
	cout << "\t-u <int> : the number of processors in each PE (0 means no limit) [default = " << nprocs << "]" << endl;
	cout << "\t-t <int> : the number of contexts for pipeline (0 means no pipelining) [default = " << ncontexts << "]" << endl;
	cout << "\t-x       : toggle enabling external memory to store intermediate values [default = " << fextmem << "]" << endl;
	cout << "\t-c       : toggle transforming dataflow [default = " << ftransform << "]" << endl;
	cout << "\t-m       : toggle using given multi-operator operations [default = " << fmultiopr << "]" << endl;
	cout << "\t-i       : toggle using ILP solver instead of SAT solver [default = " << filp << "]" << endl;
	cout << "\t-y       : toggle post processing to remove redundancy [default = " << freduce << "]" << endl;
	cout << "\t-a       : toggle doing incremental synthesis [default = " << finc << "]" << endl;
	cout << "\t-z       : toggle naming intermadiate variables in the result [default = " << fname << "]" << endl;
	cout << "\t-l <str> : the duration of timeout for each problem (0 means no time limit) [default = " << timeout << "]" << endl;
	cout << "\t-d <int> : the type of at most one encoding [default = " << nencode << "]" << endl;
	cout << "\t           \t0 : naive" << endl;
	cout << "\t           \t1 : commander" << endl;
	cout << "\t           \t2 : binary" << endl;
	cout << "\t           \t3 : bimander half" << endl;
	cout << "\t           \t4 : bimander root" << endl;
	cout << "\t-s <int> : SAT solver [default = " << nsolver << "]" << endl;
	cout << "\t           \t0 : minisat" << endl;
	cout << "\t           \t1 : glucose" << endl;
	cout << "\t           \t2 : lingeling" << endl;
	cout << "\t           \t3 : plingeling" << endl;
	cout << "\t-v <int> : the level of verbosing information [default = " << nverbose << "]" << endl;
	cout << "\t           \t0 : minimum" << endl;
	cout << "\t           \t1 : results" << endl;
	cout << "\t           \t2 : settings and results" << endl;
	cout << "\t           \t3 : settings, solver outputs, and results" << endl;
	return 0;
      default:
	show_error("invalid option", argv[i]);
      }
    }
  }

  // read array processor file
  Graph graph;
  graph.create_node("mem", "_extmem");
  graph.read(efilename);
  if(nverbose >= 2) {
    cout << "### array processor ###" << endl;
    graph.print();
  }

  // read data-flow file
  Dfg dfg;
  dfg.read(ffilename);
  if(nverbose >= 2) {
    cout << "### data-flow ###" << endl;
    dfg.print();
  }

  // apply compression
  if(ftransform) {
    dfg.compress();
    if(nverbose >= 2) {
      cout << "### data-flow after compression ###" << endl;
      dfg.print();
    }
  }

  // generate operand list
  dfg.gen_operands(fmultiopr, fname);
  if(nverbose >= 2) {
    cout << "### operand list ###" << endl;
    dfg.print_operands();
  }

  // instanciate problem generator
  for(auto type : graph.get_types()) {
    if(type != "pe" && type != "mem") {
      show_error("unknown type", type);
    }
  }
  Cnf cnf = Cnf(graph.get_nodes("pe"), graph.get_nodes("mem"), graph.get_edges("com"), dfg.get_ninputs(), dfg.output_ids(), dfg.get_operands());

  // set option
  {
    cnf.nencode = nencode;
    cnf.fmulti = dfg.get_fmulti();
    cnf.filp = filp;
    cnf.priority = dfg.get_priority();

    // read option file
    ifstream f(gfilename);
    if(!f) {
      cout << "no option file" << endl;
    }
    else {
      bool r = 1;
      string l, s;
      stringstream ss;
      vector<string> vs;
      while(1) {
	if(r) {
	  if(!getline(f, l)) {
	    break;
	  }
	  vs.clear();
	  ss.str(l);
	  ss.clear();
	  while(getline(ss, s, ' ')) {
	    vs.push_back(s);
	  }
	  if(vs.empty()) {
	    continue;
	  }
	}
	r = 1;
	if(vs[0] == ".assign") {
	  while(getline(f, l)) {
	    vs.clear();
	    ss.str(l);
	    ss.clear();
	    while(getline(ss, s, ' ')) {
	      vs.push_back(s);
	    }
	    if(vs.empty()) {
	      continue;
	    }
	    if(vs[0][0] == '.') {
	      r = 0;
	      break;
	    }
	    int id = graph.get_id(vs[0]);
	    if(id == -1) {
	      show_error("unspecified node", vs[0]);
	    }
	    cnf.assignments[id].clear();
	    cnf.assignments[id].resize(dfg.get_ndata());
	    for(int i = 1; i < (int)vs.size(); i++) {
	      if(vs[i] == "{") {
		int n;
		try {
		  n = str2int(vs[++i]);
		}
		catch(...) {
		  show_error("unspecified size", l);
		}
		if(n <= 0) {
		  show_error("non-positive size", vs[i]);
		}
		i++;
		set<int> sinputs;
		while(i < (int)vs.size() && vs[i] != "}") {
		  int idi = dfg.input_id(vs[i]);
		  cnf.assignments[id][idi] = -1;
		  sinputs.insert(idi);
		  i++;
		}
		if(vs[i] != "}") {
		  show_error("incomplete line", l);
		}
		cnf.amk_assignments.push_back(make_tuple(id, sinputs, n));
		continue;
	      }
	      cnf.assignments[id][dfg.input_id(vs[i])] = 1;
	    }
	  }
	}
	if(vs[0] == ".memsize") {
	  while(getline(f, l)) {
	    vs.clear();
	    ss.str(l);
	    ss.clear();
	    while(getline(ss, s, ' ')) {
	      vs.push_back(s);
	    }
	    if(vs.empty()) {
	      continue;
	    }
	    if(vs[0][0] == '.') {
	      r = 0;
	      break;
	    }
	    if(vs.size() != 2) {
	      show_error("incomplete line", l);
	    }
	    int id = graph.get_id(vs[0]);
	    if(id == -1) {
	      show_error("unspecified node", vs[0]);
	    }
	    if(graph.get_type(id) != "mem") {
	      show_error("non-Mem node", vs[0]);
	    }
	    try {
	      cnf.memsize[id] = str2int(vs[1]);
	    }
	    catch(...) {
	      show_error("non-integer size", vs[1]);
	    }
	    if(cnf.memsize[id] < 0) {
	      show_error("memsize must not be negative", vs[1]);
	    }
	  }
	}
	if(vs[0] == ".port") {
	  while(getline(f, l)) {
	    vs.clear();
	    ss.str(l);
	    ss.clear();
	    while(getline(ss, s, ' ')) {
	      vs.push_back(s);
	    }
	    if(vs.empty()) {
	      continue;
	    }
	    if(vs[0][0] == '.') {
	      r = 0;
	      break;
	    }
	    if(vs.size() != 3) {
	      show_error("incomplete line", l);
	    }
	    int id = graph.get_id(vs[0]);
	    if(id == -1) {
	      show_error("unspecified node", vs[0]);
	    }
	    try {
	      cnf.nports[id] = make_pair(str2int(vs[1]), str2int(vs[2]));
	    }
	    catch(...) {
	      show_error("non-integer sizes", l);
	    }
	    if(cnf.nports[id].first < 0 || cnf.nports[id].second < 0) {
	      show_error("the number of ports must not be negative", l);
	    }
	  }
	}
	if(vs[0] == ".temp") {
	  cnf.tempnodes.resize(graph.get_nnodes());
	  for(int i = 1; i < (int)vs.size(); i++) {
	    int id = graph.get_id(vs[i]);
	    if(id == -1) {
	      show_error("unspecified node", vs[i]);
	    }
	    cnf.tempnodes[id] = 1;
	  }
	}
      }
      if(!cnf.nports.empty()) {
	cnf.support_port();
      }
      if(nverbose >= 2) {
	cout << "### synthesis option ###" << endl;
	cout << "assignments :" << endl;
	for(auto i : cnf.assignments) {
	  cout << "node " << i.first << " :" << endl;
	  for(auto j : i.second) {
	    cout << j << ", ";
	  }
	  cout << endl;
	}
	cout << "memory size :" << endl;
	for(auto i : cnf.memsize) {
	  cout << "node " << i.first << " : " << i.second << endl;
	}
	cout << "the number of ports :" << endl;	
	for(auto i : cnf.nports) {
	  cout << "node " << i.first << " : " << i.second.first << ", " << i.second.second << endl;
	}
	cout << "temporary nodes :" << endl;
	if(!cnf.tempnodes.empty()) {
	  for(int i = 0; i < graph.get_nnodes(); i++) {
	    if(cnf.tempnodes[i]) {
	      cout << i << ", ";
	    }
	  }
	  cout << endl;
	}
      }
    }
  }

  // prepare parameters
  if(finc && ncycles < 1) {
    ncycles = 1;
  }
  if(ncycles < 1) {
    cout << "your files are valid" << endl;
    cout << "to run synthesis, please specify the number of cycles by using option -n" << endl;
    return 0;
  }
  string solver_cmd = solver_cmds[nsolver];
  if(filp) {
    pfilename = pfilename_ilp;
    rfilename = rfilename_ilp;
    solver_cmd = solver_cmd_ilp;
  }
  double totaltime = 0;

  // solve
  while(1) {
    cout << "### synthesis ###" << endl;
    cout << "ncycles : " << ncycles << endl;
    cout << "ndata : " << dfg.get_ndata() << endl;
    cnf.gen_cnf(ncycles, nregs, nprocs, fextmem, ncontexts, pfilename);
    string cmd;
    if(timeout != "0") {
      cmd = "timeout " + timeout + " ";
    }
    cmd += solver_cmd;
    if(nverbose < 3) {
      cmd += " > /dev/null 2>&1";
    }
    remove(rfilename.c_str());
    auto starttime = chrono::system_clock::now();
    int r = system(cmd.c_str());
    auto endtime = chrono::system_clock::now();
    r = r >> 8;
    if(r == 124) {
      cout << "Timeout" << endl;
      return 0;
    }
    if(r > 124) {
      show_error("solver command", cmd);
    }
    double dtime = chrono::duration_cast<std::chrono::milliseconds>(endtime-starttime).count();
    totaltime += dtime;
    ifstream f(rfilename);
    if(filp) {
      r = !f.fail();
    }
    else {
      if(!f) {
	show_error("cannot open result file", rfilename);
      }
      string l;
      r = -1;
      while(getline(f, l)) {
	string s;
	stringstream ss(l);
	getline(ss, s, ' ');
	if(s == "SAT" || s == "1" || s == "-1") {
	  r = 1;
	  break;
	}
	else if(s == "UNSAT") {
	  r = 0;
	  break;
	}
	else if(s == "s") {
	  getline(ss, s, ' ');
	  if(s == "SATISFIABLE") {
	    r = 1;
	    break;
	  }
	  else if(s == "UNSATISFIABLE") {
	    r = 0;
	    break;
	  }
	}
      }
      if(r == -1) {
	show_error("malformed result", rfilename);
      }
    }
    f.close();
    
    // show results
    if(r) {
      std::cout << "Solution found" << std::endl;
      cout << "time : " << dtime << "ms" << endl;
      break;
    }
    else {
      std::cout << "No solution" << std::endl;
      cout << "time : " << dtime << "ms" << endl;
    }
    if(!finc) {
      return 0;
    }
    ncycles++;
  }

  if(finc) {
    cout << endl << "time : " << totaltime << "ms in total" << endl;
  }

  cnf.gen_image(rfilename);

  if(freduce) {
    cnf.reduce_image();
  }
  
  if(nverbose) {
    map<int, string> node_id2name = graph.get_id2name();
    auto coms = graph.get_edges("com");
    for(int j = 0; j < (int)coms.size(); j++) {
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
      node_id2name[graph.get_nnodes() + j] = name;
    }
    cout << "### results ###" << endl;
    for(int k = 0; k < (int)cnf.image.size(); k++) {
      cout << "cycle " << k << " :" << endl;
      for(int j = 0; j < (int)cnf.image[0].size(); j++) {
	if(!node_id2name.count(j)) {
	  continue;
	}
	cout << "\t" << node_id2name[j] << " :";
	for(int i : cnf.image[k][j]) {
	  cout << " #" << i;
	  if(!dfg.get_dataname(i).empty()) {
	    cout << "=" << dfg.get_dataname(i);
	  }
	  cout << ", ";
	}
	cout << endl;
      }
    }
  }

  // dot file
  if(fout) {
    for(int i = 0; 1; i++) {
      string name = ifilename_base + to_string(i) + "." + ifilename_type;
      if(remove(name.c_str())) {
	break;
      }
    }
    map<int, string> node_id2name = graph.get_id2name();
    auto pes = graph.get_nodes("pe");
    auto mems = graph.get_nodes("mem");
    auto coms = graph.get_edges("com");
    for(int k = 0; k < (int)cnf.image.size(); k++) {
      ofstream f(dfilename);
      if(!f) {
	show_error("cannot open file", dfilename);
      }
      f << "digraph cycle" << k << " {" << endl;
      f << "node [ shape = record ];" << endl;
      for(auto j : pes) {
	f << "n" << j << " [ label = \"{" << node_id2name[j];
	if(!cnf.image[k][j].empty()) {
	  f << "|{";
	  for(int i : cnf.image[k][j]) {
	    f << " #" << i;
	    if(!dfg.get_dataname(i).empty()) {
	      f << "=" << dfg.get_dataname(i);
	    }
	    f << "|";
	  }
	  f.seekp(-1, ios_base::cur);
	  f << "}";
	}
	f << "}\" ];" << endl;
      }
      for(auto j : mems) {
	f << "n" << j << " [ label = \"" << node_id2name[j];
	if(!cnf.image[k][j].empty()) {
	  f << "|{";
	  for(int i : cnf.image[k][j]) {
	    f << " #" << i;
	    if(!dfg.get_dataname(i).empty()) {
	      f << "=" << dfg.get_dataname(i);
	    }
	    f << "|";
	  }
	  f.seekp(-1, ios_base::cur);
	  f << "}";
	}
	f << "\" ];" << endl;
      }
      for(int h = 0; h < (int)coms.size(); h++) {
	auto &com = coms[h];
	if(get<0>(com).size() > 1 || get<1>(com).size() > 1) {
	  if(cnf.image[k][graph.get_nnodes()+h].empty()) {
	    f << "c" << h << " [ shape = point ];" << endl;
	    for(int j : get<0>(com)) {
	      f << "n" << j << " -> c" << h << " [ style = \"dashed\" ];" << endl;
	    }
	    for(int j : get<1>(com)) {
	      f << "c" << h << " -> n" << j << " [ style = \"dashed\" ];" << endl;
	    }
	  }
	  else {
	    f << "c" << h << " [ style = \"dashed\", label =  \"{";
	    for(int i : cnf.image[k][graph.get_nnodes()+h]) {
	      f << " #" << i;
	      if(!dfg.get_dataname(i).empty()) {
		f << "=" << dfg.get_dataname(i);
	      }
	      f << "|";
	    }
	    f.seekp(-1, ios_base::cur);
	    f << "}\" ];" << endl;
	    for(int j : get<0>(com)) {
	      f << "n" << j << " -> c" << h << ";" << endl;
	    }
	    for(int j : get<1>(com)) {
	      f << "c" << h << " -> n" << j << ";" << endl;
	    }
	  }
	}
	else {
	  f << "n" << *get<0>(com).begin() << " -> n" << *get<1>(com).begin();
	  if(cnf.image[k][graph.get_nnodes()+h].empty()) {
	    f << " [ style = \"dashed\" ];" << endl;
	  }
	  else {
	    f << " [ label =  \"";
	    for(int i : cnf.image[k][graph.get_nnodes()+h]) {
	      f << " #" << i;
	      if(!dfg.get_dataname(i).empty()) {
		f << "=" << dfg.get_dataname(i);
	      }
	      f << ", ";
	    }
	    f.seekp(-2, ios_base::cur);
	    f << "\" ];" << endl;
	  }
	}
      }
      f << "}" << endl;
      f.close();
      string cmd = dot_cmd;
      cmd += " -o " + ifilename_base + to_string(k) + "." + ifilename_type;
      system(cmd.c_str());
    }
  }

  return 0;
}
