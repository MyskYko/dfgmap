#include <fstream>
#include <sstream>

#include "global.hpp"
#include "graph.hpp"
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
  
  bool fextmem = 0;
  bool ftransform = 0;
  int npipeline = 0;

  int nencode = 3;
  bool finc = 0;
  bool filp = 0;
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

  // read option file
  ifstream gfile(gfilename);
  string str;
  map<int, set<int> > assignments;
  set<int> sinputs;
  for(int i = 0; i < dfg.get_ninputs(); i++) {
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
	  int id = graph.get_id(vs[0]);
	  if(id == -1) {
	    show_error("node " + vs[0] + " does not exist");
	  }
	  if(graph.get_type(id) != "mem") {
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
  
  // instanciate problem generator
  Cnf cnf = Cnf(graph.get_nodes("pe"), graph.get_nodes("mem"), graph.get_edges("com"), dfg.get_ninputs(), dfg.output_ids(), assignments, dfg.get_operands());
  cnf.nencode = nencode;
  if(dfg.get_fmulti()) {
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
    cout << "ndata : " << dfg.get_ndata() << endl;
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
