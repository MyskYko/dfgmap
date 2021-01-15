#include <fstream>
#include <sstream>
#include <iterator>
#include <iomanip>

#include "global.hpp"
#include "dfg.hpp"

using namespace std;

Dfg::~Dfg() {
  for(auto * p : oprs) {
    delete p;
  }
  for(auto * p : nodes) {
    delete p;
  }
}

void Dfg::create_opr(string s, int n, bool fc, bool fa) {
  opr * p = new opr;
  p->s = s;
  p->n = n;
  p->attr = 0;
  if(fc) {
    p->attr += 1;
  }
  if(fa) {
    p->attr += 2;
  }
  oprs.push_back(p);
}

int Dfg::oprtype(string s) {
  for(int i = 0; i < (int)oprs.size(); i++) {
    if(oprs[i]->s == s) {
      return i;
    }
  }
  return -1;
}

string Dfg::typeopr(int i) {
  if(i < 0 || i >= (int)oprs.size()) {
    show_error("no oprtype", to_string(i) );
  }
  return oprs[i]->s;
}

bool Dfg::fcommutative(int i) {
  if(i < 0 || i >= (int)oprs.size()) {
    show_error("no oprtype", to_string(i) );
  }
  return oprs[i]->attr % 2;
}

bool Dfg::fassociative(int i) {
  if(i < 0 || i >= (int)oprs.size()) {
    show_error("no oprtype", to_string(i) );
  }
  return oprs[i]->attr >> 1;
}

void Dfg::create_input(string name) {
  datanames.push_back(name);
  node * p = new node;
  p->type = -1;
  p->id = ninputs++;
  name2node[name] = p;
  nodes.push_back(p);
}

Dfg::node *Dfg::create_multiopr(vector<string> &vs, int &pos) {
  if(pos >= (int)vs.size()) {
    ostringstream os;
    copy(vs.begin(), vs.end(), ostream_iterator<string>(os, " "));
    show_error("incomplete line", os.str());
  }
  if(oprtype(vs[pos]) < 0) {
    return NULL;
  }
  node *p = new node;
  p->id = 1;
  p->type = oprtype(vs[pos]);
  for(int i = 0; i < oprs[p->type]->n; i++) {
    node *c = create_multiopr(vs, ++pos);
    if(c) {
      p->id = 0;
    }
    p->vc.push_back(c);
  }
  nodes.push_back(p);
  return p;
}

void Dfg::create_multiopr(vector<string> &vs) {
  int pos = 0;
  node * p = create_multiopr(vs, pos);
  multioprs.push_back(p);
}

Dfg::node *Dfg::create_node(vector<string> &vs, int &pos) {
  if(pos >= (int)vs.size()) {
    ostringstream os;
    copy(vs.begin(), vs.end(), ostream_iterator<string>(os, " "));
    show_error("incomplete line", os.str());
  }
  if(oprtype(vs[pos]) < 0) {
    node *p = name2node[vs[pos]];
    if(!p) {
      show_error("unspecified data", vs[pos]);
    }
    return p;
  }
  node *p = new node;
  p->id = -1;
  p->type = oprtype(vs[pos]);
  for(int i = 0; i < oprs[p->type]->n; i++) {
    node *c = create_node(vs, ++pos);
    p->vc.push_back(c);
  }
  nodes.push_back(p);
  return p;
}

void Dfg::create_node(vector<string> &vs) {
  if(name2node.count(vs[0])) {
    show_error("duplicated data", vs[0]);
  }
  int pos = 1;
  name2node[vs[0]] = create_node(vs, pos);
}

void Dfg::print_node(node *p, int depth, int exind = 0) {
  for(int i = 0; i < depth; i++) {
    cout << "\t";
  }
  if(p->type == -1) {
    cout << p->id << endl;
  }
  else if(p->type < 0) {
    cout << p->type << "(" << p << "," << exind << ")" << endl;
  }
  else {
    cout << typeopr(p->type) << endl;
  }
  if(exind) {
    return;
  }
  for(int i = 0; i < (int)p->vc.size(); i++) {
    auto c = p->vc[i];
    if(i < (int)p->exind.size()) {
      print_node(c, depth + 1, p->exind[i]);
    }
    else {
      print_node(c, depth + 1);
    }
  }
}

void Dfg::print() {
  cout << "id to name :" << endl;
  for(int i = 0; i < (int)datanames.size(); i++) {
    cout << "\t" << i << " : " << datanames[i] << endl;
  }
  cout << "expression tree :" << endl;
  for(auto s : outputnames) {
    cout << "\t" << s << " :" << endl;
    node * p = name2node[s];
    if(!p) {
      show_error("unspecified output", s);
    }
    print_node(p, 1);
  }
}

int Dfg::input_id(string name) {
  node * p = name2node[name];
  if(!p) {
    show_error("unspecified input", name);
  }
  if(p->id < 0 || p->id >= ninputs) {
    show_error("non-input data", name);
  }
  return p->id;
}

void Dfg::compress_node(node *p) {
  if(p->type < 0) {
    return;
  }
  if(fassociative(p->type)) {
    vector<node *> vcn;
    for(int i = 0; i < (int)p->vc.size(); i++) {
      auto c = p->vc[i];
      if(!c->dependent && p->type == c->type) {
	p->vc.insert(p->vc.begin() + i + 1, c->vc.begin(), c->vc.end());
      }
      else {
	vcn.push_back(c);
      }
    }
    p->vc = vcn;
  }
  for(auto c : p->vc) {
    compress_node(c);
  }
}

void Dfg::compress() {
  for(auto s : outputnames) {
    node * p = name2node[s];
    if(!p) {
      show_error("unspecified output", s);
    }
    compress_node(p);
  }
}

void Dfg::insert_xbtree_node(node *p) {
  if(p->type < 0) {
    return;
  }
  if(p->vc.size() > 2 && fassociative(p->type) && fcommutative(p->type)) {
    node * q = new node;
    q->id = -1;
    q->type = -3;
    q->vc = p->vc;
    nodes.push_back(q);
    if(p->vc.size() == 3) {
      node * r = new node;
      r->id = -1;
      r->type = p->type;
      r->vc.push_back(q);
      r->exind.push_back(0);
      r->vc.push_back(q);
      r->exind.push_back(1);
      nodes.push_back(r);
      p->vc.clear();
      p->vc.push_back(q);
      p->exind.push_back(2);
      p->vc.push_back(r);
    }
    else if(p->vc.size() == 4) {
      node * r = new node;
      r->id = -1;
      r->type = p->type;
      r->vc.push_back(q);
      r->exind.push_back(0);
      r->vc.push_back(q);
      r->exind.push_back(1);
      nodes.push_back(r);
      node * s = new node;
      s->id = -1;
      s->type = -2;
      s->vc.push_back(q);
      s->exind.push_back(2);
      s->vc.push_back(r);
      nodes.push_back(s);
      node * t = new node;
      t->id = -1;
      t->type = p->type;
      t->vc.push_back(q);
      t->exind.push_back(3);
      t->vc.push_back(s);
      t->exind.push_back(0);
      nodes.push_back(t);
      p->vc.clear();
      p->vc.push_back(s);
      p->exind.push_back(1);
      p->vc.push_back(t);
    }
    else if(p->vc.size() == 5) {
      node * r = new node;
      r->id = -1;
      r->type = p->type;
      r->vc.push_back(q);
      r->exind.push_back(0);
      r->vc.push_back(q);
      r->exind.push_back(1);
      nodes.push_back(r);
      node * s = new node;
      s->id = -1;
      s->type = -2;
      s->vc.push_back(q);
      s->exind.push_back(2);
      s->vc.push_back(r);
      nodes.push_back(s);
      node * t = new node;
      t->id = -1;
      t->type = p->type;
      t->vc.push_back(q);
      t->exind.push_back(3);
      t->vc.push_back(s);
      t->exind.push_back(0);
      nodes.push_back(t);
      node * u = new node;
      u->id = -1;
      u->type = -2;
      u->vc.push_back(q);
      u->exind.push_back(4);
      u->vc.push_back(t);
      nodes.push_back(u);
      node * v = new node;
      v->id = -1;
      v->type = p->type;
      v->vc.push_back(s);
      v->exind.push_back(1);
      v->vc.push_back(u);
      v->exind.push_back(0);
      nodes.push_back(v);
      p->vc.clear();
      p->vc.push_back(u);
      p->exind.push_back(1);
      p->vc.push_back(v);
    }
    else if(p->vc.size() == 6) {
      node * r = new node;
      r->id = -1;
      r->type = p->type;
      r->vc.push_back(q);
      r->exind.push_back(0);
      r->vc.push_back(q);
      r->exind.push_back(1);
      nodes.push_back(r);
      node * s = new node;
      s->id = -1;
      s->type = -2;
      s->vc.push_back(q);
      s->exind.push_back(2);
      s->vc.push_back(r);
      nodes.push_back(s);
      node * t = new node;
      t->id = -1;
      t->type = p->type;
      t->vc.push_back(q);
      t->exind.push_back(3);
      t->vc.push_back(s);
      t->exind.push_back(0);
      nodes.push_back(t);
      node * u = new node;
      u->id = -1;
      u->type = -2;
      u->vc.push_back(q);
      u->exind.push_back(4);
      u->vc.push_back(t);
      nodes.push_back(u);
      node * v = new node;
      v->id = -1;
      v->type = p->type;
      v->vc.push_back(s);
      v->exind.push_back(1);
      v->vc.push_back(u);
      v->exind.push_back(0);
      nodes.push_back(v);
      node * w = new node;
      w->id = -1;
      w->type = -2;
      w->vc.push_back(q);
      w->exind.push_back(5);
      w->vc.push_back(v);
      nodes.push_back(w);
      node * x = new node;
      x->id = -1;
      x->type = p->type;
      x->vc.push_back(u);
      x->exind.push_back(1);
      x->vc.push_back(w);
      x->exind.push_back(0);
      nodes.push_back(x);
      p->vc.clear();
      p->vc.push_back(w);
      p->exind.push_back(1);
      p->vc.push_back(x);
    }
    else if(p->vc.size() == 7) {
      node * r = new node;
      r->id = -1;
      r->type = p->type;
      r->vc.push_back(q);
      r->exind.push_back(0);
      r->vc.push_back(q);
      r->exind.push_back(1);
      nodes.push_back(r);
      node * s = new node;
      s->id = -1;
      s->type = -2;
      s->vc.push_back(q);
      s->exind.push_back(2);
      s->vc.push_back(q);
      s->exind.push_back(3);
      s->vc.push_back(r);
      nodes.push_back(s);
      node * t = new node;
      t->id = -1;
      t->type = p->type;
      t->vc.push_back(q);
      t->exind.push_back(4);
      t->vc.push_back(s);
      t->exind.push_back(0);
      nodes.push_back(t);
      node * u = new node;
      u->id = -1;
      u->type = -2;
      u->vc.push_back(q);
      u->exind.push_back(5);
      u->vc.push_back(t);
      nodes.push_back(u);
      node * v = new node;
      v->id = -1;
      v->type = p->type;
      v->vc.push_back(s);
      v->exind.push_back(1);
      v->vc.push_back(u);
      v->exind.push_back(0);
      nodes.push_back(v);
      node * w = new node;
      w->id = -1;
      w->type = -2;
      w->vc.push_back(q);
      w->exind.push_back(6);
      w->vc.push_back(v);
      nodes.push_back(w);
      node * x = new node;
      x->id = -1;
      x->type = p->type;
      x->vc.push_back(u);
      x->exind.push_back(1);
      x->vc.push_back(w);
      x->exind.push_back(0);
      nodes.push_back(x);
      node * y = new node;
      y->id = -1;
      y->type = -2;
      y->vc.push_back(s);
      y->exind.push_back(2);
      y->vc.push_back(x);
      nodes.push_back(y);
      node * z = new node;
      z->id = -1;
      z->type = p->type;
      z->vc.push_back(w);
      z->exind.push_back(1);
      z->vc.push_back(y);
      z->exind.push_back(0);
      nodes.push_back(z);
      p->vc.clear();
      p->vc.push_back(y);
      p->exind.push_back(1);
      p->vc.push_back(z);
    }
    else {
      show_error("currently more than 7 input xbtree is not supported");
    }
    p = q;
  }
  for(auto c : p->vc) {
    insert_xbtree_node(c);
  }
}

void Dfg::insert_xbtree() {
  for(auto s : outputnames) {
    node * p = name2node[s];
    if(!p) {
      show_error("unspecified output", s);
    }
    insert_xbtree_node(p);
  }
}

void Dfg::gen_operands_node(node *p, bool fname) {
  if(p->id != -1) {
    return;
  }
  vector<int> cids;
  for(int i = 0; i < (int)p->vc.size(); i++) {
    auto c = p->vc[i];
    gen_operands_node(c, fname);
    if(c->type < -1) {
      cids.push_back(-(c->id + 1 + p->exind[i]));
    }
    else {
      cids.push_back(c->id);
    }
  }
  if(p->type < -1) {
    p->id = nexs;
    exs.push_back(make_tuple(p->type == -3, nexs, nsels, cids));
    nexs += cids.size();
    if(p->type == -2) {
      nsels += cids.size();
    }
    else if(p->type == -3) {
      nsels += cids.size() * cids.size();
    }
    else {
      show_error("unexpected error");
    }
    return;
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
    oprtypes.push_back(p->type);
    dependents.push_back(p->dependent);
    operands_.push_back(sv);
    string dataname;
    if(fname) {
      dataname += typeopr(p->type);
      dataname += " ";
      for(auto id : cids) {
	if(id < 0) {
	  dataname += "__ex" + to_string(-id-1);
	}
	else {
	  dataname += datanames[id];
	}
	dataname += " ";
      }
      dataname = dataname.substr(0, dataname.size()-1);
    }
    datanames.push_back(dataname);
    unique[key] = ndata++;
    p->id = unique[key];    
    return;
  }
  if(!fcommutative(p->type)) {
    for(int i = 2; i <= (int)cids.size(); i++) {
      for(int j = 0; j <= (int)cids.size()-i; j++) {
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
	oprtypes.push_back(p->type);
	dependents.push_back(p->dependent);
	operands_.push_back(sv);
	string dataname;
	if(fname) {
	  dataname += typeopr(p->type);
	  dataname += " ";
	  for(auto id : sub) {
	    if(id < 0) {
	      dataname += "__ex" + to_string(-id-1);
	    }
	    else {
	      dataname += datanames[id];
	    }
	    dataname += " ";
	  }
	  dataname = dataname.substr(0, dataname.size()-1);
	}
	datanames.push_back(dataname);
	unique[keysub] = ndata++;
      }
    }
    p->id = unique[key];
    return;
  }
  for(int i = 2; i <= (int)cids.size(); i++) {
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
				   oprtypes.push_back(p->type);
				   dependents.push_back(p->dependent);
				   operands_.push_back(sv);
				   string dataname;
				   if(fname) {
				     dataname += typeopr(p->type);
				     dataname += " ";
				     for(auto id : sub) {
				       if(id < 0) {
					 dataname += "__ex" + to_string(-id-1);
				       }
				       else {
					 dataname += datanames[id];
				       }
				       dataname += " ";
				     }
				     dataname = dataname.substr(0, dataname.size()-1);
				   }
				   datanames.push_back(dataname);
				   unique[keysub] = ndata++;
				 });
  }
  p->id = unique[key];
}

void Dfg::gen_operands(bool fmultiopr, bool fname) {
  ndata = ninputs;
  oprtypes.clear();
  oprtypes.resize(ndata);
  dependents.clear();
  dependents.resize(ndata);
  operands_.clear();
  operands_.resize(ndata);
  unique.clear();
  for(auto s : outputnames) {
    node * p = name2node[s];
    if(!p) {
      show_error("unspecified output", s);
    }
    gen_operands_node(p, fname);
  }
  fmulti = 0;
  operands.clear();
  operands.resize(ndata);
  exconds.clear();
  exconds.resize(ndata);
  if(fmultiopr) {
    support_multiopr();
  }
  for(int i = 0; i < ndata; i++) {
    for(auto &v : operands_[i]) {
      set<int> s(v.begin(), v.end());
      map<int, bool> m;
      operands[i].push_back(s);
      exconds[i].push_back(m);
    }
    set<pair<set<int>, map<int, bool> > > s;
    for(int j = 0; j < (int)operands[i].size(); j++) {
      s.insert(make_pair(operands[i][j], exconds[i][j]));
    }
    operands[i].clear();
    exconds[i].clear();
    for(auto e: s) {
      operands[i].push_back(e.first);
      exconds[i].push_back(e.second);
    }
    if(!fmulti && operands[i].size() > 1) {
      fmulti = 1;
    }
  }
}

bool Dfg::support_multiopr_rec(int id, node *ope, vector<set<int> > &vs, vector<map<int, bool> > &vm) {
  if(ope == NULL) {
    for(auto &s : vs) {
      s.insert(id);
    }
    return 1;
  }
  if(id >= 0 && (dependents[id] || oprtypes[id] != ope->type)) {
    return 0;
  }
  vector<set<int> > vs_;
  vector<map<int, bool> > vm_;
  if(id < 0) {
    id = -id - 1;
    int exid = exs.size() - 1;
    for(int i = 0; i < (int)exs.size(); i++) {
      if(std::get<1>(exs[i]) > id) {
	exid = i - 1;
	break;
      }
    }
    auto &ex = exs[exid];
    int ind = id - std::get<1>(ex);
    int nc = std::get<3>(ex).size();
    for(int ii = 0; ii < nc; ii++) {
      vector<set<int> > vs2(1);
      vector<map<int, bool> > vm2(1);
      int s = std::get<2>(ex);
      if(std::get<0>(ex)) {
	for(int ii_ = 0; ii_ < nc; ii_++) {
	  vm2[0][s + ind * nc + ii_] = ii == ii_;
	}
	for(int i = 0; i < nc; i++) {
	  vm2[0][s + i * nc + ii] = i == ind;
	}
      }
      else {
	for(int ii_ = 0; ii_ < nc; ii_++) {
	  vm2[0][s + ii_] = ii_ == (ii - ind + nc) % nc;
	}
      }
      bool r = support_multiopr_rec(std::get<3>(ex)[ii], ope, vs2, vm2);
      if(!r) {
	continue;
      }
      for(int i = 0; i < (int)vs2.size(); i++) {
	vs_.push_back(vs2[i]);
	vm_.push_back(vm2[i]);
      }
    }
  }
  else {
    for(auto &v : operands_[id]) {
      if(fcommutative(ope->type) && !ope->id) {
	foreach_perm(oprs[ope->type]->n, [&](int *indices) {
	  vector<set<int> > vs2(1);
	  vector<map<int, bool> > vm2(1);
	  for(int i = 0; i < oprs[ope->type]->n; i++) {
	    bool r = support_multiopr_rec(v[indices[i]], ope->vc[i], vs2, vm2);
	    if(!r) {
	      return;
	    }
	  }
	  for(int i = 0; i < (int)vs2.size(); i++) {
	    vs_.push_back(vs2[i]);
	    vm_.push_back(vm2[i]);
	  }
	});
      }
      else {
	vector<set<int> > vs2(1);
	vector<map<int, bool> > vm2(1);
	bool r = 0;
	for(int i = 0; i < oprs[ope->type]->n; i++) {
	  r = support_multiopr_rec(v[i], ope->vc[i], vs2, vm);
	  if(!r) {
	    break;
	  }
	}
	if(!r) {
	  continue;
	}
	for(int i = 0; i < (int)vs2.size(); i++) {
	  vs_.push_back(vs2[i]);
	  vm_.push_back(vm2[i]);
	}
      }
    }
  }
  vector<set<int> > vsnew;
  vector<map<int, bool> > vmnew;
  for(int i = 0; i < (int)vs.size(); i++) {
    for(int j = 0; j < (int)vs_.size(); j++) {
      map<int, bool> m = vm[i];
      bool f = 0;
      for(auto e: vm_[j]) {
	if(m.count(e.first) && m[e.first] != e.second) {
	  f = 1;
	  break;
	}
	m[e.first] = e.second;
      }
      if(f) {
	continue;
      }
      set<int> s = vs[i];
      s.insert(vs_[j].begin(), vs_[j].end());
      vsnew.push_back(s);
      vmnew.push_back(m);
    }
  }
  if(vsnew.empty()) {
    return 0;
  }
  vs = vsnew;
  vm = vmnew;
  return 1;
}

void Dfg::support_multiopr() {
  for(auto multiopr : multioprs) {
    for(int i = 0; i < ndata; i++) { 
      vector<set<int> > vs(1);
      vector<map<int, bool> > vm(1);
      bool tmp = dependents[i];
      dependents[i] = 0;
      bool r = support_multiopr_rec(i, multiopr, vs, vm);
      dependents[i] = tmp;
      if(r) {
	for(int j = 0; j < (int)vs.size(); j++) {
	  operands[i].push_back(vs[j]);
	  exconds[i].push_back(vm[j]);
	}
      }
    }
  }
}

set<int> Dfg::output_ids() {
  set<int> ids;
  for(auto s : outputnames) {
    node * p = name2node[s];
    if(!p) {
      show_error("unspecified output", s);
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
    if(vs[0] == ".i") {
      for(int i = 1; i < (int)vs.size(); i++) {
	create_input(vs[i]);
      }
    }
    if(vs[0] == ".o") {
      for(int i = 1; i < (int)vs.size(); i++) {
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
	  n = str2int(vs[1]);
	}
	catch(...) {
	  show_error("non-integer noperands", vs[1]);
	}
	if(n <= 0) {
	  show_error("negative noperands", vs[1]);
	}
	bool fc = 0;
	bool fa = 0;
	for(int i = 2; i < (int)vs.size(); i++) {
	  if(vs[i] == "c") {
	    fc = 1;
	  }
	  if(vs[i] == "a") {
	    fa = 1;
	  }
	}
	create_opr(vs[0], n, fc, fa);
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
	create_multiopr(vs);
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
	create_node(vs);
      }
    }
    if(vs[0] == ".p") {
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
	if(vs.size() < 3) {
	  show_error("incomplete line", l);
	}
	node * p, * q;
	p = name2node[vs[0]];
	if(!p) {
	  show_error("unspecified data", vs[0]);
	}
	p->dependent = true;
	for(int i = 1; i < (int)vs.size(); i += 2) {
	  q = name2node[vs[i+1]];
	  if(!q) {
	    show_error("unspecified data", vs[i+1]);
	  }
	  q->dependent = true;
	  if(vs[i] == ">") {
	    priority.push_back(make_tuple(p, q, false));
	  }
	  else if(vs[i] == ">=") {
	    priority.push_back(make_tuple(p, q, true));
	  }
	  else if(vs[i] == "<") {
	    priority.push_back(make_tuple(q, p, false));
	  }
	  else if(vs[i] == "<=") {
	    priority.push_back(make_tuple(q, p, true));
	  }
	  else if(vs[i] == "=") {
	    priority.push_back(make_tuple(p, q, true));
	    priority.push_back(make_tuple(q, p, true));
	  }
	  else {
	    show_error("unknown comparison operator", vs[i]);
	  }
	  p = q;
	}
      }
    }
  }
  f.close();
}

vector<tuple<int, int, bool> > Dfg::get_priority() {
  vector<tuple<int, int, bool> > v;
  for(auto &t : priority) {
    v.push_back(make_tuple((get<0>(t))->id, (get<1>(t))->id, get<2>(t)));
  }
  return v;
}

void Dfg::print_operands() {
  for(int i = ninputs; i < ndata; i++) {
    auto &a = operands[i];
    cout << i << " :" << endl;
    for(int j = 0; j < (int)a.size(); j++) {
      auto &b = a[j];
      cout << "\t";
      for(auto c : b) {
	cout << setw(3) << c << ", ";
      }
      auto &d = exconds[i][j];
      if(!d.empty()) {
	cout << "\t(";
	for(auto c : d) {
	  if(c.second) {
	    cout << setw(3) << c.first << ", ";
	  }
	}
	cout << ")";
      }
      cout << endl;
    }
  }
}

void Dfg::update_datanames(map<int, int> &exmap) {
  for(int i = 0; i < ndata; i++) {
    if(datanames[i].empty()) {
      continue;  
    }
    vector<string> vs;
    {
      stringstream ss;
      string s;
      ss.str(datanames[i]);
      while(getline(ss, s, ' ')) {
	if(s.size() > 4 && s.substr(0, 4) == "__ex") {
	  vs.push_back(s);
	}
      }
    }
    for(string s: vs) {
      int exid = str2int(s.substr(4));
      datanames[i].replace(datanames[i].find(s), s.size(), datanames[exmap[exid]]);
    }
  }
}
