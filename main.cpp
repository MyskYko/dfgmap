#include <fstream>
#include <sstream>
#include <cstdio>

#include "global.hpp"
#include "graph.hpp"
#include "dfg.hpp"
#include "cnf.hpp"

using namespace std;

int main(int argc, char** argv) {
  string efilename = "e.txt";
  string ffilename = "f.txt";
  string gfilename = "g.txt";
  string pfilename = "_test.cnf";
  string rfilename = "_test.out";

  vector<string> solver_cmds = {"minisat " + pfilename + " " + rfilename,
				"glucose " + pfilename + " " + rfilename,
				"lingeling " + pfilename + " | tee " + rfilename,
				"plingeling " + pfilename + " | tee " + rfilename};

  string pfilename_ilp = "_test.lp";
  string rfilename_ilp = "_test.sol";
  string solver_cmd_ilp = "cplex -c \"read " + pfilename_ilp + "\" \"set emphasis mip 1\" \"set threads 1\" \"optimize\" \"write " +  rfilename_ilp + "\"";

  
  int ncycles = 0;
  int nregs = 2;
  int nprocs = 1;
  
  bool fextmem = 0;
  bool freduce = 1;
  bool ftransform = 0;
  int ncontexts = 0;

  int nsolver = 0;
  int nencode = 3;
  bool finc = 0;
  bool filp = 0;

  string timeout = "1d";
  
  int nverbose = 0;

  // read command
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
	  ncycles = str2int(argv[++i]);
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
	  nregs = str2int(argv[++i]);
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
	  nprocs = str2int(argv[++i]);
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
      case 'y':
	freduce ^= 1;
	break;
      case 'c':
	ftransform ^= 1;
	break;
      case 't':
	try {
	  ncontexts = str2int(argv[++i]);
	}
	catch(...) {
	  show_error("-t must be followed by integer");
	}
	if(ncontexts <= 0) {
	  show_error("the number of contexts must be more than 0");
	}
	break;
      case 'a':
	finc ^= 1;
	break;
      case 'i':
	filp ^= 1;
	break;
      case 'l':
	try {
	  timeout = argv[++i];
	  char c = timeout.back();
	  if(c == 's' || c == 'm' || c == 'h' || c == 'd') {
	    str2int(timeout.substr(0, timeout.size()-1));
	  }
	  else {
	    str2int(timeout);
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
	if(nsolver < 0 || nsolver >= solver_cmds.size()) {
	  show_error("SAT solver must be more than 0 and less than", to_string(solver_cmds.size()));
	}
	break;
      case 'v':
	if(i+1 >= argc || argv[i+1][0] == '-') {
	  nverbose = 1;
	  break;
	}
	try {
	  nverbose = str2int(argv[++i]);
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
	cout << "\t-y       : toggle enabling post processing to remove redundancy [default = " << freduce << "]" << endl;
	cout << "\t-c       : toggle transforming dataflow [default = " << ftransform << "]" << endl;
	cout << "\t-t <int> : the number of contexts for pipeline (0 means no pipelining) [default = " << ncontexts << "]" << endl;
	cout << "\t-a       : toggle incremental synthesis [default = " << finc << "]" << endl;
	cout << "\t-i       : toggle using ILP solver instead of SAT solver [default = " << filp << "]" << endl;
	cout << "\t-l <str> : timeout duration (0 means no time limit) [default = " << timeout << "]" << endl;
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
	cout << "\t           \t0 : nothing" << endl;
	cout << "\t           \t1 : results" << endl;
	cout << "\t           \t2 : settings and results" << endl;
	cout << "\t           \t3 : settings, solver outputs, and results" << endl;
	return 0;
      default:
	show_error("invalid option", argv[i]);
      }
    }
  }

  // read environment file
  Graph graph;
  graph.create_node("mem", "_extmem");
  graph.read(efilename);
  if(nverbose >= 2) {
    cout << "### environment information ###" << endl;
    graph.print();
  }

  // read formula file
  Dfg dfg;
  dfg.read(ffilename);
  if(nverbose >= 2) {
    cout << "### formula information ###" << endl;
    dfg.print();
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
	    if(graph.get_type(id) != "mem") {
	      show_error("non-Mem node", vs[0]);
	    }
	    set<int> sinputs;
	    for(int i = 1; i < vs.size(); i++) {
	      sinputs.insert(dfg.input_id(vs[i]));
	    }
	    cnf.assignments[id] = sinputs;
	  }
	}
      }
      if(nverbose >= 2) {
	cout << "### option information ###" << endl;
	cout << "assignments :" << endl;
	for(auto i : cnf.assignments) {
	  cout << "node " << i.first << " :" << endl;
	  for(auto j : i.second) {
	    cout << j << ", ";
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

  // solve
  while(1) {
    cout << "ncycles : " << ncycles << endl;
    cout << "ndata : " << dfg.get_ndata() << endl;
    cnf.gen_cnf(ncycles, nregs, nprocs, fextmem, ncontexts, pfilename);
    string cmd;
    if(timeout != "0") {
      cmd = "timeout 1d ";
    }
    cmd += solver_cmd;
    if(nverbose < 3) {
      cmd += " > /dev/null 2>&1";
    }
    remove(rfilename.c_str());
    int r = system(cmd.c_str());
    r = r >> 8;
    if(r == 124) {
      cout << "Timeout" << endl;
      return 0;
    }
    if(r > 124) {
      show_error("solver command", cmd);
    }
    ifstream f(rfilename);
    if(filp) {
      if(!f) {
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
    if(!f) {
      show_error("cannot open result file", rfilename);
    }
    r = 0;
    string l;
    while(getline(f, l)) {
      string s;
      stringstream ss(l);
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
    f.close();
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

  if(freduce) {
    cnf.reduce_image();
  }
  
  if(nverbose) {
    map<int, string> node_id2name = graph.get_id2name();
    auto coms = graph.get_edges("com");
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
      node_id2name[graph.get_nnodes() + j] = name;
    }
    cout << "### results ###" << endl;
    for(int k = 0; k < cnf.image.size(); k++) {
      cout << "cycle " << k << " :" << endl;
      for(int j = 0; j < cnf.image[0].size(); j++) {
	cout << "\t" << node_id2name[j] << " :";
	for(int i : cnf.image[k][j]) {
	  cout << " " << i << "#" << dfg.get_dataname(i);
	}
	cout << endl;
      }
    }
  }

  return 0;
}
