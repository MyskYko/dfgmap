#include <fstream>
#include <sstream>
#include <cassert>
#include <algorithm>

#include "dfg.hpp"

using namespace std;

int Dfg::add_operator(string s, int n) {
  opr * p = new opr;
  p->s = s;
  p->n = n;
  p->attr = 0;
  oprs.push_back(p);
  return oprs.size() - 1;
}

int Dfg::optype(string s) {
  for(int i = 0; i < oprs.size(); i++) {
    if(oprs[i]->s == s) {
      return i;
    }
  }
  return -1;
}

string Dfg::typeop(int i) {
  if(i < 0 || i >= oprs.size()) {
    show_error("no operator at " + to_string(i) );
  }
  return oprs[i]->s;
}

int Dfg::fcommutative(int i) {
  if(i < 0 || i >= oprs.size()) {
    show_error("no operator at " + to_string(i) );
  }
  return oprs[i]->attr % 2;
}

int Dfg::fassociative(int i) {
  if(i < 0 || i >= oprs.size()) {
    show_error("no operator at " + to_string(i) );
  }
  return (oprs[i]->attr >> 1) % 2;
}

void Dfg::make_commutative(int i) {
  if(fcommutative(i)) {
    show_error("operator " + to_string(i) + " is already commutative");
  }
  oprs[i]->attr += 1;
}

void Dfg::make_associative(int i) {
  if(fassociative(i)) {
    show_error("operator " + to_string(i) + " is already associative");
  }
  if(oprs[i]->n != 2) {
    show_error("associative operator must have 2 inputs");
  }
  oprs[i]->attr += 2;
}

void Dfg::create_input(string name) {
  datanames.push_back(name);
  opnode * p = new opnode;
  p->type = -1;
  p->id = ninputs++;
  data_name2opnode[name] = p;
  opnodes.push_back(p);
}

Dfg::opnode *Dfg::create_multiope(vector<string> &vs, int &pos) {
  if(pos >= vs.size()) {
    show_error("formula file has an incomplete line");
  }
  if(optype(vs[pos]) < 0) {
    return NULL;
  }
  opnode *p = new opnode;
  p->id = 1;
  p->type = optype(vs[pos]);
  for(int i = 0; i < oprs[p->type]->n; i++) {
    opnode *c = create_multiope(vs, ++pos);
    if(c) {
      p->id = 0;
    }
    p->vc.push_back(c);
  }
  opnodes.push_back(p);
  return p;
}

void Dfg::add_multiope(vector<string> &vs) {
  int pos = 0;
  opnode * p = create_multiope(vs, pos);
  multiopes.push_back(p);
}

Dfg::opnode *Dfg::create_opnode(vector<string> &vs, int &pos) {
  if(pos >= vs.size()) {
    show_error("formula file has an incomplete line");
  }
  if(optype(vs[pos]) < 0) {
    opnode *p = data_name2opnode[vs[pos]];
    if(!p) {
      show_error("data " + vs[pos] + " unspecified");
    }
    return p;
  }
  opnode *p = new opnode;
  p->id = -1;
  p->type = optype(vs[pos]);
  for(int i = 0; i < oprs[p->type]->n; i++) {
    opnode *c = create_opnode(vs, ++pos);
    p->vc.push_back(c);
  }
  opnodes.push_back(p);
  return p;
}

void Dfg::create_opnode(vector<string> &vs) {
  if(data_name2opnode.count(vs[0])) {
    show_error("data name in formula duplicated");
  }
  int pos = 1;
  data_name2opnode[vs[0]] = create_opnode(vs, pos);
}

void Dfg::print_opnode(opnode *p, int depth) {
  for(int i = 0; i < depth; i++) {
    cout << "\t";
  }
  if(p->type < 0) {
    cout << p->id << endl;
  }
  else {
    cout << typeop(p->type) << endl;
  }
  for(auto c : p->vc) {
    print_opnode(c, depth + 1);
  }
}

void Dfg::print() {
  for(auto s : outputnames) {
    opnode * p = data_name2opnode[s];
    if(!p) {
      show_error("output data " + s + " function unspecified");
    }
    print_opnode(p, 0);
  }
}

int Dfg::input_id(string name) {
  opnode * p = data_name2opnode[name];
  if(!p) {
    show_error("input " + name + " unspecified");
  }
  if(p->id < 0 || p->id >= ninputs) {
    show_error("data " + name + " is not input");
  }
  return p->id;
}

void Dfg::compress_opnode(opnode *p) {
  if(p->type < 0) {
    return;
  }
  if(fassociative(p->type)) {
    vector<opnode *> vcn;
    for(int i = 0; i < p->vc.size(); i++) {
      auto c = p->vc[i];
      if(p->type == c->type) {
	p->vc.insert(p->vc.begin() + i + 1, c->vc.begin(), c->vc.end());
      }
      else {
	vcn.push_back(c);
      }
    }
    p->vc = vcn;
  }
  for(auto c : p->vc) {
    compress_opnode(c);
  }
}

void Dfg::compress() {
  for(auto s : outputnames) {
    opnode * p = data_name2opnode[s];
    if(!p) {
      show_error("output data " + s + " function unspecified");
    }
    compress_opnode(p);
  }
}

void Dfg::gen_operands_opnode(opnode *p) {
  if(p->id != -1) {
    return;
  }
  vector<int> cids;
  for(auto c : p->vc) {
    gen_operands_opnode(c);
    cids.push_back(c->id);
  }
  if(fcommutative(p->type)) {
    sort(cids.begin(), cids.end());
  }
  pair<int, vector<int> > key = make_pair(p->type, cids);
  if(unique[key]) {
    p->id = unique[key];
    return;
  }
  if(!fassociative(p->type)) {
    set<vector<int> > sv;
    sv.insert(cids);
    optypes.push_back(p->type);
    voperands.push_back(sv);
    string dataname;
    dataname += typeop(p->type);
    dataname += "(";
    for(auto id : cids) {
      dataname += datanames[id];
      dataname += ", ";
    }
    dataname.pop_back();
    dataname.pop_back();
    dataname += ")";
    datanames.push_back(dataname);
    unique[key] = ndata++;
    p->id = unique[key];    
    return;
  }
  if(!fcommutative(p->type)) {
    for(int i = 2; i <= cids.size(); i++) {
      for(int j = 0; j <= cids.size()-i; j++) {
	vector<int> sub;
	for(int k = j; k < j+i; k++) {
	  sub.push_back(cids[k]);
	}
	pair<int, vector<int> > keysub = make_pair(p->type, sub);
	if(unique[keysub]) {
	  continue;
	}
	set<vector<int> > sv;	
	for(int k = 1; k < i; k++) {
	  vector<int> v;
	  vector<int> a;
	  vector<int> b;
	  for(int l = 0; l < k; l++) {
	    a.push_back(sub[l]);
	  }
	  for(int l = k; l < i; l++) {
	    b.push_back(sub[l]);	    
	  }
	  if(a.size() == 1) {
	    v.push_back(a[0]);
	  }
	  else {
	    pair<int, vector<int> > keya = make_pair(p->type, a);
	    v.push_back(unique[keya]);
	  }
	  if(b.size() == 1) {
	    v.push_back(b[0]);
	  }
	  else {
	    pair<int, vector<int> > keyb = make_pair(p->type, b);
	    v.push_back(unique[keyb]);
	  }
	  sort(v.begin(), v.end());
	  sv.insert(v);	  
	}
	optypes.push_back(p->type);
	voperands.push_back(sv);
	string dataname;
	for(auto id : sub) {
	  dataname += datanames[id];
	  dataname += " ";
	  dataname += typeop(p->type);
	  dataname += " ";
	}
	dataname.pop_back();
	dataname.pop_back();
	dataname.pop_back();
	datanames.push_back(dataname);
	unique[keysub] = ndata++;
      }
    }
    p->id = unique[key];
    return;
  }
  for(int i = 2; i <= cids.size(); i++) {
    foreach_comb(cids.size(), i, [&](int *indices) {
				   vector<int> sub;
				   for(int k = 0; k < i; k++) {
				     sub.push_back(cids[indices[k]]);
				   }
				   sort(sub.begin(), sub.end());
				   pair<int, vector<int> > keysub = make_pair(p->type, sub);
				   if(unique[keysub]) {
				     return;
				   }
				   set<vector<int> > sv;
				   for(int j = 1; j < (1 << (i-1)); j++) {
				     vector<int> v;
				     vector<int> a;
				     vector<int> b;
				     int j_ = j;
				     for(int k = 0; k < i; k++) {
				       if(j_ % 2) {
					 a.push_back(sub[k]);
				       }
				       else {
					 b.push_back(sub[k]);
				       }
				       j_ = j_ >> 1;
				     }
				     if(a.size() == 1) {
				       v.push_back(a[0]);
				     }
				     else {
				       sort(a.begin(), a.end());
				       pair<int, vector<int> > keya = make_pair(p->type, a);
				       v.push_back(unique[keya]);
				     }
				     if(b.size() == 1) {
				       v.push_back(b[0]);
				     }
				     else {
				       sort(b.begin(), b.end());
				       pair<int, vector<int> > keyb = make_pair(p->type, b);
				       v.push_back(unique[keyb]);
				     }
				     sort(v.begin(), v.end());
				     sv.insert(v);
				   }
				   optypes.push_back(p->type);
				   voperands.push_back(sv);
				   string dataname;
				   for(auto id : sub) {
				     dataname += datanames[id];
				     dataname += " ";
				     dataname += typeop(p->type);
				     dataname += " ";
				   }
				   dataname.pop_back();
				   dataname.pop_back();
				   dataname.pop_back();
				   datanames.push_back(dataname);
				   unique[keysub] = ndata++;
				 });
  }
  p->id = unique[key];
}

void Dfg::gen_operands() {
  ndata = ninputs;
  optypes.clear();
  optypes.resize(ndata);
  voperands.clear();
  voperands.resize(ndata);
  unique.clear();
  for(auto s : outputnames) {
    opnode * p = data_name2opnode[s];
    if(!p) {
      show_error("output data " + s + " function unspecified");
    }
    gen_operands_opnode(p);
  }
  fmulti = 0;
  operands.clear();
  operands.resize(voperands.size());
  support_multiope();
  for(int i = 0; i < voperands.size(); i++) {
    for(auto &v : voperands[i]) {
      set<int> s(v.begin(), v.end());
      operands[i].insert(s);
    }
    if(!fmulti && operands[i].size() > 1) {
      fmulti = 1;
    }
  }
}

int Dfg::support_multiope_rec(int id, opnode *ope, vector<set<int> > &vs) {
  if(ope == NULL) {
    for(auto &s : vs) {
      s.insert(id);
    }
    return 1;
  }
  if(optypes[id] != ope->type) {
    return 0;
  }
  set<set<int> > ss;
  for(auto &v : voperands[id]) {
    if(fcommutative(ope->type) && !ope->id) {
      foreach_perm(oprs[ope->type]->n, [&](int *indices) {
			 vector<set<int> > vs2(1);
			 for(int i = 0; i < oprs[ope->type]->n; i++) {
			   int r = support_multiope_rec(v[indices[i]], ope->vc[i], vs2);
			   if(!r) {
			     return;
			   }
			 }
			 ss.insert(vs2.begin(), vs2.end());
		       });
    }
    else {
      vector<set<int> > vs2(1);
      int r = 0;
      for(int i = 0; i < oprs[ope->type]->n; i++) {
	r = support_multiope_rec(v[i], ope->vc[i], vs2);
	if(!r) {
	  break;
	}
      }
      if(!r) {
	continue;
      }
      ss.insert(vs2.begin(), vs2.end());
    }
  }
  if(ss.empty()) {
    return 0;
  }
  if(ss.size() == 1) {
    for(auto &s : vs) {
      s.insert(ss.begin()->begin(), ss.begin()->end());
    }
    return 1;
  }
  vector<set<int> > vs_ = vs;
  vs.clear();
  for(auto &s2 : ss) {
    for(auto s : vs_) {
      s.insert(s2.begin(), s2.end());
      vs.push_back(s);
    }
  }
  return 1;
}

void Dfg::support_multiope() {
  assert(optypes.size() == voperands.size());
  for(auto multiope : multiopes) {
    for(int i = 0; i < optypes.size(); i++) {
      vector<set<int> > vs(1);
      int r;
      r = support_multiope_rec(i, multiope, vs);
      if(r) {
	operands[i].insert(vs.begin(), vs.end());
      }
    }
  }
}

set<int> Dfg::output_ids() {
  set<int> ids;
  for(auto s : outputnames) {
    opnode * p = data_name2opnode[s];
    if(!p) {
      show_error("output data " + s + " function unspecified");
    }
    ids.insert(p->id);
  }
  return ids;
}

void Dfg::read(string filename) {
  ifstream f(filename);
  if(!f) {
    show_error("cannot open file", filename);
  }
  int r = 1;
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
    if(vs[0] == ".i") {
      for(int i = 1; i < vs.size(); i++) {
	create_input(vs[i]);
      }
    }
    if(vs[0] == ".o") {
      for(int i = 1; i < vs.size(); i++) {
	outputnames.push_back(vs[i]);
      }
    }
    if(vs[0] == ".f") {
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
	if(vs.size() < 2) {
	  show_error("incomplete line", l);
	}
	int n;
	try {
	  n = stoi(vs[1]);
	}
	catch(...) {
	  show_error("non-integer noperands", vs[1]);
	}
	n = add_operator(vs[0], n);
	for(int i = 2; i < vs.size(); i++) {
	  if(vs[i] == "c") {
	    make_commutative(n);
	  }
	  if(vs[i] == "a") {
	    make_associative(n);
	  }
	}
      }
    }
    if(vs[0] == ".m") {
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
	add_multiope(vs);
      }
    }
    if(vs[0] == ".n") {
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
	create_opnode(vs);
      }
    }
  }
  f.close();
}
