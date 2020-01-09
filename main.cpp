#include <fstream>
#include <sstream>
#include <iomanip>
#include <time.h>
#include <cassert>

#include "global.hpp"
#include "op.hpp"
#include "blif.hpp"

using namespace std;


int main(int argc, char** argv) {
  string efilename = "e.txt";
  string ffilename = "f.txt";
  string pfilename = "p.txt";
  string specfilename = "_spec.blif";
  string tmplfilename = "_tmpl.blif";
  string topfilename = "_top.blif";
  string logfilename = "_log.txt";

  int ncycles = 0;
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
	cout << "\t-p <str> : the name of parameter file [default = \"" << pfilename << "\"]" << endl;
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
  int nregs;
  int nops;
  vector<opnode *> operators;
  map<string, int> node_name2id;
  int nnodes = 0;
  vector<pair<int, int> > coms;
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
    if(vs[0] == ".r") {
      try {
	nregs = stoi(vs[1]);
      } catch(...) {
	show_error(".r must be followed by integer");
      }
    }
    if(vs[0] == ".op") {
      try {
	nops = stoi(vs[1]);
      } catch(...) {
	show_error(".op must be followed by integer");
      }
      map<string, opnode *> name2opnode;
      for(int i = 0; i < nops; i++) {
	char c = 'a' + i;
	string name{c};
	opnode * p = new opnode;
	p->type = 0;
	p->id = i;
	name2opnode[name] = p;
      }
      while(getline(efile, str)) {
	vs.clear();
	stringstream ss(str);
	while(getline(ss, s, ' ')) {
	  vs.push_back(s);
	}
	if(vs.empty()) {
	  continue;
	}
	if(vs[0][0] == '.') {
	  break;
	}
	int pos = 0;
	opnode * p = create_opnode(vs, pos, name2opnode);
	operators.push_back(p);
      }
    }
    if(vs[0] == ".pe") {
      for(int i = 1; i < vs.size(); i++) {
	node_name2id[vs[i]] = nnodes;
	nnodes++;
      }
    }
    if(vs[0] == ".com") {
      while(getline(efile, str)) {
	vs.clear();
	stringstream ss(str);
	while(getline(ss, s, ' ')) {
	  vs.push_back(s);
	}
	if(vs.empty()) {
	  continue;
	}
	if(vs[0][0] == '.') {
	  show_error(".com should be the last information");
	}
	int id0 = node_name2id[vs[0]];
	int id1 = node_name2id[vs[1]];
	coms.push_back(make_pair(id0, id1));
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
    cout << "coms :" << endl;
    for(auto com : coms) {
      cout << com.first << " -> " << com.second << endl;;
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

  Blif blif(datanames, outputnames);
  blif.gen_spec(specfilename, outputs);

  blif.gen_tmpl(tmplfilename, ncycles, nregs, nnodes, nops, operators, coms);

  blif.gen_top(topfilename, specfilename, tmplfilename);

  string cmd = "abc -c \"read " + topfilename + "; strash; qbf -v -P " + to_string(blif.nsels) + "\" > " + logfilename;
  system(cmd.c_str());

  vector<int> result;
  ifstream lfile(logfilename);
  if(!lfile) {
    show_error("cannot open log file");
  }
  while(getline(lfile, str)) {
    string s;
    stringstream ss(str);
    vector<string> vs;
    while(getline(ss, s, ' ')) {
      vs.push_back(s);
    }
    if(vs.empty()) {
      continue;
    }
    if(vs[0] == "Parameters:") {
      for(int i = 0; i < vs[1].size(); i++) {
	char c = vs[1][i];
	if(c == '0') {
	  result.push_back(0);
	}
	else if(c == '1') {
	  result.push_back(1);
	}
	else {
	  show_error("wrong format result");
	}
      }
    }
  }

  if(result.empty()) {
    cout << "UNSAT" << endl;
    return 0;
  }
  
  if(nverbose) {
    cout << "### result ###" << endl;
    blif.show_result(result);
  }
  
  return 0;
}
