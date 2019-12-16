#include <fstream>
#include <sstream>
#include <iomanip>
#include <time.h>

#include "global.hpp"
#include "op.hpp"
#include "sat.hpp"

using namespace std;


int main(int argc, char** argv) {
  string efilename = "e.txt";
  string ffilename = "f.txt";
  string pfilename;
  string cfilename;
  int fcompress = 0;
  int fmac = 1;
  int fexmem = 0;
  int finc = 1;
  int ncycles = 0;
  int nregs = 0;
  int nverbose = 0;

  // read options
  for(int i = 1; i < argc; i++) {
    if(argv[i][0] == '-') {
      switch(argv[i][1]) {
      case 'n':
	try {
	  ncycles = stoi(argv[++i]);
	} catch(...) {
	  show_error("-n must be followed by integer");
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
      case 'p':
	if(i+1 >= argc) {
	  show_error("-p must be followed by file name");
	}
	pfilename = argv[++i];
	break;
      case 'w':
	if(i+1 >= argc) {
	  show_error("-w must be followed by file name");
	}
	cfilename = argv[++i];
	break;
      case 'c':
	fcompress ^= 1;
	break;
      case 'm':
	fmac ^= 1;
	break;
      case 'x':
	fexmem ^= 1;
	break;
      case 't':
	finc ^= 1;
	break;
      case 'r':
	if(i+1 >= argc || argv[i+1][0] == '-') {
	  nregs = -1;
	  break;
	}
	try {
	  nregs = stoi(argv[++i]);
	} catch(...) {
	  show_error("-r should be followed by integer");
	}
	break;
      case 'v':
	if(i+1 >= argc || argv[i+1][0] == '-') {
	  nverbose = 1;
	  break;
	}
	try {
	  nverbose = stoi(argv[++i]);
	} catch(...) {
	  show_error("-v should be followed by integer");
	}
	break;
      case 'h':
	cout << "usage : gen <options>" << endl;
	cout << "\t-h       : show this usage" << endl;
	cout << "\t-n <int> : the number of cycles [default = " << ncycles << "]" << endl;
	cout << "\t-e <str> : the name of environment file [default = \"" << efilename << "\"]" << endl;
	cout << "\t-f <str> : the name of formula file [default = \"" << ffilename << "\"]" << endl;
	cout << "\t-p <str> : the name of placement file to generate pngs [default = \"" << pfilename << "\"]" << endl;
	cout << "\t-w <str> : the name of cnf file to dump without sysnthesis [default = \"" << cfilename << "\"]" << endl;
	cout << "\t-c       : toggle transforming dataflow [default = " << fcompress << "]" << endl;
	cout << "\t-m       : toggle using MAC operation [default = " << fmac << "]" << endl;
	cout << "\t-x       : toggle using external memory to store intermediate data [default = " << fexmem << "]" << endl;
	cout << "\t-t       : toggle incremental synthesis [default = " << finc << "]" << endl;
	cout << "\t-r <int> : the number of additional registers for each PE (minus will be treated as no limit) [default = " << nregs << "]" << endl;
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
  map<string, int> node_name2id;
  int nnodes = 1; // external memory
  vector<int> i_nodes;
  vector<int> o_nodes;
  vector<int> pe_nodes;
  vector<set<int> > cons;
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
    if(vs[0] == ".i") {
      for(int i = 1; i < vs.size(); i++) {
	node_name2id[vs[i]] = nnodes;
	i_nodes.push_back(nnodes);
	nnodes++;
      }
    } else if(vs[0] == ".o") {
      for(int i = 1; i < vs.size(); i++) {
	node_name2id[vs[i]] = nnodes;
	o_nodes.push_back(nnodes);
	nnodes++;
      }
    } else if(vs[0] == ".pe") {
      for(int i = 1; i < vs.size(); i++) {
	node_name2id[vs[i]] = nnodes;
	pe_nodes.push_back(nnodes);
	nnodes++;
      }
    } else {
      if(cons.size() < nnodes) {
	cons.resize(nnodes);
      }
      int id0 = node_name2id[vs[0]];
      if(!id0) {
	show_error("node " + vs[0] + " unspecified");
      }
      for(int i = 1; i < vs.size(); i++) {
	int idi = node_name2id[vs[i]];
	if(!idi) {
	  show_error("node " + vs[i] + " unspecified");
	}
	cons[id0].insert(idi);
      }
    }
  }
  efile.close();

  if(nverbose >= 2) {
    cout << "### environment information ###" << endl;
    cout << "node name to id :" << endl;
    for(auto i : node_name2id) {
      cout << i.first << " -> " << i.second << endl;;
    }
    cout << "inputs :" << endl;
    for(int i : i_nodes) {
      cout << i << ",";
    }
    cout << endl;
    cout << "outputs :" << endl;
    for(int i : o_nodes) {
      cout << i << ",";
    }
    cout << endl;
    cout << "PEs :" << endl;
    for(int i : pe_nodes) {
    cout << i << ",";
    }
    cout << endl;
    cout << "connections :" << endl;
    for(int i = 1; i < nnodes; i++) {
      cout << i << " <- ";
      for(int j : cons[i]) {
	cout << j << ",";
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
    } else if(vs[0] == ".o") {
      for(int i = 1; i < vs.size(); i++) {
	outputnames.push_back(vs[i]);
      }
    } else {
      int pos = 1;
      opnode * p = create_opnode(vs, pos, data_name2opnode);
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

  // apply compression
  if(fcompress) {
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
  vector<set<set<int> > > operands(ndata);
  map<pair<int, multiset<int> >, int> unique;
  set<int> output_ids;
  for(auto p : outputs) {
    gen_operands(p, ndata, operands, unique, datanames, fmac);
    output_ids.insert(p->id);
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

  // instanciate SAT solver
  Sat sat = Sat(i_nodes, o_nodes, pe_nodes, cons, ninputs, output_ids, operands);
  
  if(finc) {
    ncycles = 1;
  }
  
  if(ncycles < 1) {
    cout << "your files are valid" << endl;
    cout << "to run synthesis, please specify the number of cycles by using option -n" << endl;
    return 0;
  }

  vector<double> timestamp;
  while(1) {
    cout << "ncycles : " << ncycles << endl;
    // generate cnf
    if(fexmem) {
      if(nregs) {
	sat.gen_cnf_reg_exmem(ncycles, nregs);
      } else {    
	sat.gen_cnf_exmem(ncycles);
      }
    } else if(nregs) {
      sat.gen_cnf_reg(ncycles, nregs);
    } else {
      sat.gen_cnf(ncycles);
    }
    
    // write cnf file
    if(!cfilename.empty()) {
      sat.write(cfilename);
      return 0;
    }
    
    // run sat solver
    clock_t start = clock();
    bool r = sat.solve();
    clock_t end = clock();
    
    // show results
    double duration = (double)(end - start) / CLOCKS_PER_SEC;
    timestamp.push_back(duration);
    std::cout << "SAT solver took " << duration << "s" << std::endl;
    if(r) {
      std::cout << "SAT" << std::endl;
      break;
    } else {
      std::cout << "UNSAT" << std::endl;
      if(!finc) {
	return 0;
      }
    }
    ncycles++;
    sat.clean();
  }
  
  sat.gen_image();
  
  if(nverbose) {
    cout << "### results ###" << endl;
    for(int i = 0; i < sat.image.size(); i++) {
      cout << "cycle " << i << " :" << endl;
      for(int j = 0; j < sat.image[0].size(); j++) {
	cout << "\tnode " << j << " :";
	for(int k : sat.image[i][j]) {
	  cout << " " << k << "(" << datanames[k] << ")";
	}
	cout << endl;
      }
    }
  }

  // prepare for image generation
  if(pfilename.empty()) {
    return 0;
  }

  // read placement file
  ifstream pfile(pfilename);
  if(!pfile) {
    show_error("cannot open placement file");
  }
  vector<vector<string> > pl;
  while(getline(pfile, str)) {
    string s;
    stringstream ss(str);
    vector<string> vs;
    while(getline(ss, s, ' ')) {
      vs.push_back(s);
    }
    pl.push_back(vs);
  }
  pfile.close();

  time_t ctime = time(NULL);
  string outdir = "out" + to_string(ctime);
  string cmd = "mkdir " + outdir;
  system(cmd.c_str());

  // generate image
  for(int i = 0; i < ncycles; i++) {
    // generate dot file
    ofstream df("out.dot");
    df << "graph G {" << endl;
    df << "node";
    df << " [" << endl;
    df << "shape=\"box\"";
    df << "," << endl;
    df << "fixedsize=true";
    df << "," << endl;
    df << "fontsize=10";
    df << "," << endl;
    df << "labelloc=\"t\"";
    df << endl;
    df << "];" << endl;
    df << endl;
    for(auto line : pl) {
      if(line.empty()) {
	continue;
      }
      int id = node_name2id[line[0]];
      assert(id);
      float x = stof(line[1]);
      float y = stof(line[2]);
      float w = stof(line[3]);
      float h = stof(line[4]);
      x = (x + w/2) * 72;
      y = (y + h/2) * 72;
      df << line[0];
      df << " [" << endl;
      df << "pos=\"" << x << "," << y << "\"";
      df << "," << endl;
      df << "width=" << w;
      df << "," << endl;
      df << "height=" << h;
      df << "," << endl;
      if(sat.image[i][id].empty()) {
	df << "label=\"\"";
      } else {
	df << "label=\"";
	int j = 0;
	int k = 0;
	for(int l = 0; l < sat.image[i][id].size(); l++) {
	  int d = sat.image[i][id][l]; 
	  string dataname = datanames[d];
	  if(l != sat.image[i][id].size()-1) {
	    dataname += ", ";
	  }
	  for(char c : dataname) {
	    if(j >= 14*w) {
	      df << "\\l";
	      j = 0;
	      k++;
	      if(k >= 6*h - 1) {
		df << "...";
		l = sat.image[i][id].size();
		break;
	      }
	    }
	    df << c;
	    j++;
	  }
	}
	df << "\\l";
	df << "\"";
      }
      df << endl;
      df << "];" << endl;
    }
    df << "}" << endl;
    df.close();

    // generate png file
    string cmd = "neato out.dot -n -T png -o " + outdir + "/out" + to_string(i) + ".png";
    system(cmd.c_str());
  }

  string lfilename = outdir + "/log.txt";
  ofstream lfile(lfilename);
  for(int i = 0; i < argc; i++) {
    lfile << argv[i] << " ";
  }
  lfile << endl;
  for(double t : timestamp) {
    lfile << t << endl;
  }
  lfile.close();
  
  cout << "pngs are dumped at " << outdir << endl;
  
  return 0;
}
