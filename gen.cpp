//g++  -I ~/glucose-syrup gen.cpp ~/glucose-syrup/simp/lib.a -o gen
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <algorithm>
#include <iomanip>
#include <set>
#include <time.h>
#include <functional>

#include "utils/System.h"
#include "utils/ParseUtils.h"
#include "utils/Options.h"
#include "core/Dimacs.h"
#include "simp/SimpSolver.h"

#include <cassert>

using namespace std;

void show_error(string s) {
  cout << "error : " << s << endl;
  cout << "see usage by using option -h" << endl;
}

typedef struct opnode_ {
  int type; // input 0, + 1, * 2
  vector<struct opnode_ *> vc;
  int id;
} opnode;

opnode * create_opnode(vector<string> &vs, int &pos, map<string, opnode *> &input_name2opnode) {
  if(vs[pos] != "+" && vs[pos] != "*") {
    opnode * p = input_name2opnode[vs[pos]];
    if(!p) {
      show_error("data " + vs[pos] + " unspecified");
      return NULL;
    }
    return p;
  }
  opnode * p = new opnode;
  p->id = -1;
  if(vs[pos] == "+") {
    p->type = 1;
  }
  else if(vs[pos] == "*") {
    p->type = 2;
  }
  else {
    assert(0);
  }
  opnode * l = create_opnode(vs, ++pos, input_name2opnode);
  if(!l) {
    return NULL;
  }
  p->vc.push_back(l);
  opnode * r = create_opnode(vs, ++pos, input_name2opnode);
  if(!r) {
    return NULL;
  }
  p->vc.push_back(r);
  return p;
}

void print_opnode(opnode * p, int depth) {
  for(int i = 0; i < depth; i++) {
    cout << "\t";
  }
  if(p->type == 0) {
    cout << p->id << endl;
  }
  else if(p->type == 1) {
    cout << "+" << endl;
  }
  else if(p->type == 2) {
    cout << "*" << endl;
  }
  for(auto c : p->vc) {
    print_opnode(c, depth + 1);
  }
}

void compress_opnode(opnode * p) {
  if(p->type == 0) {
    return;
  }
  vector<opnode *> vcn;
  for(int i = 0; i < p->vc.size(); i++) {
    auto c = p->vc[i];
    if(p->type == c->type) {
      for(auto cc : c->vc) {
	p->vc.push_back(cc);
      }
      delete c;
    }
    else {
      vcn.push_back(c);
    }
  }
  p->vc = vcn;
  for(auto c : p->vc) {
    compress_opnode(c);
  }
}

void recursive_comb(int *indexes, int s, int rest, std::function<void(int *)> f) {
  if (rest == 0) {
    f(indexes);
  } else {
    if (s < 0) return;
    recursive_comb(indexes, s - 1, rest, f);
    indexes[rest - 1] = s;
    recursive_comb(indexes, s - 1, rest - 1, f);
  }
}

void foreach_comb(int n, int k, std::function<void(int *)> f) {
  int indexes[k];
  recursive_comb(indexes, n - 1, k, f);
}

void gen_operands(opnode * p, int &ndata, vector<set<set<int> > > &operands, map<pair<int, multiset<int> >, int> &unique, vector<string> &datanames, int fmac) {
  if(p->id != -1) {
    return;
  }
  multiset<int> cids;
  for(auto c : p->vc) {
    gen_operands(c, ndata, operands, unique, datanames, fmac);
    assert(c->id != -1);
    cids.insert(c->id);
  }
  pair<int, multiset<int> > key = make_pair(p->type, cids);
  if(unique[key]) {
    p->id = unique[key];
    return;
  }
  for(int i = 2; i <= cids.size(); i++) {
    foreach_comb(cids.size(), i, [&](int *indexes) {
				   multiset<int> sub;
				   for(int k = 0; k < i; k++) {
				     sub.insert(p->vc[indexes[k]]->id);
				   }
				   pair<int, multiset<int> > keysub = make_pair(p->type, sub);
				   if(!unique[keysub]) {
				     set<set<int> > ss;
				     for(int j = 1; j < 1 << (i-1); j++) {
				       set<int> s;
				       int j_ = j;
				       vector<opnode *> a;
				       vector<opnode *> b;
				       for(int k = 0; k < i; k++) {
					 if(j_ % 2) {
					   a.push_back(p->vc[indexes[k]]);
					 }
					 else {
					   b.push_back(p->vc[indexes[k]]);
					 }
					 j_ = j_ >> 1;
				       }
				       multiset<int> as;
				       for(auto q : a) {
					 as.insert(q->id);
				       }
				       pair<int, multiset<int> > keya = make_pair(p->type, as);
				       multiset<int> bs;
				       for(auto q : b) {
					 bs.insert(q->id);
				       }
				       pair<int, multiset<int> > keyb = make_pair(p->type, bs);
				       if(a.size() == 1) {
					 s.insert(a[0]->id);
				       }
				       else {
					 s.insert(unique[keya]);
				       }
				       if(b.size() == 1) {
					 s.insert(b[0]->id);
				       }
				       else {
					 s.insert(unique[keyb]);
				       }
				       ss.insert(s);
				       // MAC
				       if(fmac) {
					 if(a.size() == 1 && p->type == 1 && a[0]->type == 2) {
					   for(auto cs : operands[a[0]->id]) {
					     s.clear();
					     for(auto cc : cs) {
					       s.insert(cc);
					     }
					     if(b.size() == 1) {
					       s.insert(b[0]->id);
					     }
					     else {
					       s.insert(unique[keyb]);
					     }
					     ss.insert(s);
					   }
					 }
					 if(b.size() == 1 && p->type == 1 && b[0]->type == 2) {
					   for(auto cs : operands[b[0]->id]) {
					     s.clear();
					     for(auto cc : cs) {
					       s.insert(cc);
					     }
					     if(a.size() == 1) {
					       s.insert(a[0]->id);
					     }
					     else {
					       s.insert(unique[keya]);
					     }
					     ss.insert(s);
					   }
					 }
				       }
				     }
				     operands.push_back(ss);
				     string dataname;
				     for(auto id : sub) {
				       dataname += datanames[id];
				       if(p->type == 1) {
					 dataname += " + ";
				       }
				       else if(p->type == 2) {
					 dataname += " * ";
				       }
				     }
				     dataname.pop_back();
				     dataname.pop_back();
				     dataname.pop_back();
				     datanames.push_back(dataname);
				     unique[keysub] = ndata++;
				   }
				 });
  }
  p->id = unique[key];
}

int cardinality(Glucose::SimpSolver &S, int i, int n) {
  vector<int> vVars;
  for(int j = i; j < i+n; j++) {
    vVars.push_back(j);
  }
  while(vVars.size() > 1) {
    int k = 0;
    for(int j = 0; j < vVars.size()/2; j++) {
      S.newVar();
      Glucose::Lit l = Glucose::mkLit(S.nVars()-1);
      Glucose::Lit l0 = Glucose::mkLit(vVars[2*j]);
      Glucose::Lit l1 = Glucose::mkLit(vVars[2*j+1]);
      S.addClause(~l0, ~l1);
      S.addClause(~l, l0, l1);
      S.addClause(~l0, l);
      S.addClause(~l1, l);
      vVars[k++] = S.nVars()-1;
    }
    if(vVars.size()%2) {
      vVars[k++] = vVars.back();
    }
    vVars.resize(k);
  }
  return 0;
}

int cardinality_set(Glucose::SimpSolver &S, int i, set<int> s) {
  vector<int> vVars;
  for(int j : s) {
    vVars.push_back(i+j);
  }
  while(vVars.size() > 1) {
    int k = 0;
    for(int j = 0; j < vVars.size()/2; j++) {
      S.newVar();
      Glucose::Lit l = Glucose::mkLit(S.nVars()-1);
      Glucose::Lit l0 = Glucose::mkLit(vVars[2*j]);
      Glucose::Lit l1 = Glucose::mkLit(vVars[2*j+1]);
      S.addClause(~l0, ~l1);
      S.addClause(~l, l0, l1);
      S.addClause(~l0, l);
      S.addClause(~l1, l);
      vVars[k++] = S.nVars()-1;
    }
    if(vVars.size()%2) {
      vVars[k++] = vVars.back();
    }
    vVars.resize(k);
  }
  return 0;
}

int main(int argc, char** argv) {
  string efilename = "e.txt";
  string ffilename = "f.txt";
  int ncycle = 0;
  string plfilename;
  int fcompress = 0;
  int fmac = 0;
  int fexmem = 0;
  int nverbose = 0;

  // read options
  for(int i = 1; i < argc; i++) {
    if(argv[i][0] == '-') {
      switch(argv[i][1]) {
      case 'n':
	try {
	  ncycle = stoi(argv[++i]);
	} catch(...) {
	  show_error("-n must be followed by integer");
	  return 1;
	}
	break;
      case 'e':
	if(i+1 >= argc) {
	  show_error("-e must be followed by file name");
	  return 1;
	}
	efilename = argv[++i];
	break;
      case 'f':
	if(i+1 >= argc) {
	  show_error("-f must be followed by file name");
	  return 1;
	}
	ffilename = argv[++i];
	break;
      case 'p':
	if(i+1 >= argc) {
	  show_error("-p must be followed by file name");
	  return 1;
	}
	plfilename = argv[++i];
	break;
      case 'c':
	fcompress = 1;
	break;
      case 'm':
	fmac = 1;
	break;
      case 't':
	fexmem = 1;
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
	  return 1;
	}
	break;
      case 'h':
	cout << "usage : tobe constructed" << endl;
	return 0;
      default:
	show_error("invalid option " + string(argv[i]));
	return 1;
      }
    }
  }

  // read environment file
  ifstream efile(efilename);
  if(!efile) {
    show_error("cannot open environment file");
    return 1;
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
    }
    else if(vs[0] == ".o") {
      for(int i = 1; i < vs.size(); i++) {
	node_name2id[vs[i]] = nnodes;
	o_nodes.push_back(nnodes);
	nnodes++;
      }
    }
    else if(vs[0] == ".pe") {
      for(int i = 1; i < vs.size(); i++) {
	node_name2id[vs[i]] = nnodes;
	pe_nodes.push_back(nnodes);
	nnodes++;
      }
    }
    else {
      if(cons.size() < nnodes) {
	cons.resize(nnodes);
      }
      int id0 = node_name2id[vs[0]];
      if(!id0) {
	show_error("node " + vs[0] + " unspecified");
	return 1;
      }
      for(int i = 1; i < vs.size(); i++) {
	int idi = node_name2id[vs[i]];
	if(!idi) {
	  show_error("node " + vs[i] + " unspecified");
	  return 1;
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
    return 1;
  }
  int ninputs = 0;
  map<string, opnode *> input_name2opnode;
  vector<string> datanames;
  vector<opnode *> inputs(1, NULL);
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
	inputs.push_back(p);
	input_name2opnode[vs[i]] = p;
      }
    }
    else {
      int pos = 0;
      opnode * p = create_opnode(vs, pos, input_name2opnode);
      if(!p) {
	return 1;
      }
      outputs.push_back(p);
    }
  }
  ffile.close();

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

  if(ncycle < 1) {
    cout << "your files are valid" << endl;
    cout << "to run synthesis, please specify the number of cycles by using option -n";
    return 0;
  }
  
  // instanciate SAT solver
  Glucose::SimpSolver S;
  while(ncycle * nnodes * ndata > S.nVars()) {
    S.newVar();
  }
  
  // init condition
  for(int i = 0; i < nnodes; i++) {
    for(int j = 0; j < ndata; j++) {
      S.addClause(Glucose::mkLit(i*ndata + j, true));
    }
  }

  // conditions for each cycle
  for(int i = 1; i < ncycle; i++) {
    
    // conditions for input nodes
    for(int j : i_nodes) {
      cardinality(S, i*nnodes*ndata + j*ndata, ninputs);
      for(int k = ninputs; k < ndata; k++) {
	S.addClause(Glucose::mkLit(i*nnodes*ndata + j*ndata + k, true));
      }
    }
    
    // conditions for output nodes
    for(int j : o_nodes) {
      cardinality_set(S, i*nnodes*ndata + j*ndata, output_ids);
      for(int k : output_ids) {
	Glucose::Lit l = Glucose::mkLit(i*nnodes*ndata + j*ndata + k);
	Glucose::vec<Glucose::Lit> ls;
	ls.push(~l);
	for(int f : cons[j]) {
	  Glucose::Lit lf = Glucose::mkLit((i-1)*nnodes*ndata + f*ndata + k);
	  ls.push(lf);
	}
	S.addClause(ls);
      }
      for(int k = 0; k < ndata; k++) {
	if(find(output_ids.begin(), output_ids.end(), k) == output_ids.end()) {	
	  S.addClause(Glucose::mkLit(i*nnodes*ndata + j*ndata + k, true));
	}
      }
    }
    
    // conditions for external memory
    for(int k = 0; k < ndata; k++) {
      Glucose::Lit l = Glucose::mkLit(i*nnodes*ndata + k);
      Glucose::vec<Glucose::Lit> ls;
      ls.push(~l);
      Glucose::Lit lp = Glucose::mkLit((i-1)*nnodes*ndata + k);
      S.addClause(~lp, l);
      ls.push(lp);
      for(int j : o_nodes) {
	Glucose::Lit lo = Glucose::mkLit(i*nnodes*ndata + j*ndata + k);
	S.addClause(~lo, l);
	ls.push(lo);
      }
      S.addClause(ls);
    }
    
    // conditions for PE nodes
    for(int j : pe_nodes) {
      cardinality(S, i*nnodes*ndata + j*ndata, ndata);
      
      // create OR of existence of data among adjacent nodes and itself
      vector<int> vVars;
      for(int k = 0; k < ndata; k++) {
	S.newVar();
	Glucose::Lit l = Glucose::mkLit(S.nVars()-1);
	Glucose::vec<Glucose::Lit> ls;
	ls.push(~l);
	Glucose::Lit lself = Glucose::mkLit((i-1)*nnodes*ndata + j*ndata + k);
	ls.push(lself);
	S.addClause(~lself, l);
	for(int f : cons[j]) {
	  Glucose::Lit lf = Glucose::mkLit((i-1)*nnodes*ndata + f*ndata + k);
	  ls.push(lf);
	  S.addClause(~lf, l);
	}
	S.addClause(ls);
	vVars.push_back(S.nVars()-1);
      }
      
      // conditions for communication and operation
      for(int k = 0; k < ndata; k++) {
	Glucose::Lit l = Glucose::mkLit(i*nnodes*ndata + j*ndata + k);
	Glucose::vec<Glucose::Lit> lt;
	lt.push(~l);
	
	// operation possibility
	for(auto s : operands[k]) {
	  S.newVar();
	  Glucose::Lit la = Glucose::mkLit(S.nVars()-1);
	  Glucose::vec<Glucose::Lit> ls;
	  ls.push(la);
	  for(int o : s) {
	    Glucose::Lit lo = Glucose::mkLit(vVars[o]);
	    ls.push(~lo);
	    S.addClause(~la, lo);
	  }
	  S.addClause(ls);
	  lt.push(la);
	}
	
	// communication possibility
	Glucose::Lit lk = Glucose::mkLit(vVars[k]);
	lt.push(lk);
	S.addClause(lt);
      }
    }
  }

  // conditions for output ready
  for(int k : output_ids) {
    Glucose::Lit l = Glucose::mkLit((ncycle-1)*nnodes*ndata + k);
    S.addClause(l);
  }

  // run sat solver
  clock_t start = clock();
  bool r = S.solve();
  clock_t end = clock();
  std::cout << "SAT solver took " << (double)(end - start) / CLOCKS_PER_SEC << "s" << std::endl;

  // check results
  if(r) {
    std::cout << "SAT" << std::endl;
    /*
    ofstream sat_file("test");
    sat_file << "SAT" << std::endl;
    for(int i = 0; i < S.nVars(); i++) {
      if(S.model[i] == l_True) {
	sat_file << i + 1 << " ";
      }
      else {
	sat_file << "-" << i + 1 << " ";
      }
    }
    */
  }
  else {
    std::cout << "UNSAT" << std::endl;
    return 0;
  }

  if(nverbose) {
    cout << "### results ###" << endl;
    for(int i = 0; i < ncycle; i++) {
      cout << "cycle " << i << " :" << endl;
      for(int j = 0; j < nnodes; j++) {
	cout << "\tnode " << j << " :";
	for(int k = 0; k < ndata; k++) {
	  if(S.model[i*nnodes*ndata + j*ndata + k] == l_True) {
	    cout << " " << k << "(" << datanames[k] << ")";
	  }
	}
	cout << endl;
      }
    }
  }

  // prepare for image generation
  if(plfilename.empty()) {
    return 0;
  }

  // create data distribution among nodes for each cycle
  vector<vector<vector<int> > > image(ncycle, vector<vector<int> >(nnodes));
  for(int i = 0; i < ncycle; i++) {
    for(int j = 0; j < nnodes; j++) {
      for(int k = 0; k < ndata; k++) {
	if(S.model[i*nnodes*ndata + j*ndata + k] == l_True) {
	  image[i][j].push_back(k);
	}
      }
    }
  }

  // read placement file
  ifstream plfile(plfilename);
  if(!plfile) {
    show_error("cannot open placement file");
    return 1;
  }
  vector<vector<string> > pl;
  while(getline(plfile, str)) {
    string s;
    stringstream ss(str);
    vector<string> vs;
    while(getline(ss, s, ' ')) {
      vs.push_back(s);
    }
    pl.push_back(vs);
  }
  plfile.close();

  // generate image
  for(int i = 0; i < ncycle; i++) {
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
      if(image[i][id].empty()) {
	df << "label=\"\"";
      }
      else {
	df << "label=\"";
	int j = 0;
	int k = 0;
	for(char c : datanames[image[i][id][0]]) {
	  if(j >= 14*w) {
	    df << "\\l";
	    j = 0;
	    k++;
	    if(k >= 6*h - 1) {
	      df << "...";
	      break;
	    }
	  }
	  df << c;
	  j++;
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
    string cmd = "neato out.dot -n -T png -o out" + to_string(i) + ".png";
    system(cmd.c_str());
  }
  return 0;
}
