#include <cassert>
#include <algorithm>

#include "op.hpp"

using namespace std;

int Dfg::add_operator(string op, int n) {
  operators.push_back(op);
  noperands.push_back(n);
  assert(operators.size() == noperands.size());
  return operators.size() - 1;
}

int Dfg::optype(string s) {
  auto it = find(operators.begin(), operators.end(), s);
  if(it == operators.end()) {
    return -1;
  }
  return distance(operators.begin(), it);
}

string Dfg::typeop(int i) {
  if(i < 0 || i >= operators.size()) {
    show_error("no operator at " + to_string(i) );
  }
  return operators[i];
}

int Dfg::fcompressible(int i) {
  auto it = find(vcompressible.begin(), vcompressible.end(), i);
  if(it == vcompressible.end()) {
    return 0;
  }
  assert(noperands[i] == 2);
  return 1;
}

void Dfg::create_input(string name) {
  datanames.push_back(name);
  opnode * p = new opnode;
  p->type = -1;
  p->id = ninputs++;
  data_name2opnode[name] = p;
  opnodes.push_back(p);
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
  for(int i = 0; i < noperands[p->type]; i++) {
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
  if(fcompressible(p->type)) {
    vector<opnode *> vcn;
    for(int i = 0; i < p->vc.size(); i++) {
      auto c = p->vc[i];
      if(p->type == c->type) {
	for(auto cc : c->vc) {
	  p->vc.push_back(cc);
	}
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
  multiset<int> cids;
  for(auto c : p->vc) {
    gen_operands_opnode(c);
    cids.insert(c->id);
  }
  pair<int, multiset<int> > key = make_pair(p->type, cids);
  if(unique[key]) {
    p->id = unique[key];
    return;
  }
  if(!fcompressible(p->type)) {
    set<set<int> > ss;
    set<int> s(cids.begin(), cids.end());
    ss.insert(s);
    optypes.push_back(p->type);
    operands.push_back(ss);
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
  for(int i = 2; i <= cids.size(); i++) {
    foreach_comb(cids.size(), i, [&](int *indexes) {
				   multiset<int> sub;
				   for(int k = 0; k < i; k++) {
				     sub.insert(p->vc[indexes[k]]->id);
				   }
				   pair<int, multiset<int> > keysub = make_pair(p->type, sub);
				   if(unique[keysub]) {
				     return;
				   }
				   set<set<int> > ss;
				   for(int j = 1; j < 1 << (i-1); j++) {
				     set<int> s;
				     multiset<int> a;
				     multiset<int> b;
				     int j_ = j;
				     for(int k = 0; k < i; k++) {
				       if(j_ % 2) {
					 a.insert(p->vc[indexes[k]]->id);
				       }
				       else {
					 b.insert(p->vc[indexes[k]]->id);
				       }
				       j_ = j_ >> 1;
				     }
				     if(a.size() == 1) {
				       s.insert(*a.begin());
				     }
				     else {
				       pair<int, multiset<int> > keya = make_pair(p->type, a);
				       s.insert(unique[keya]);
				     }
				     if(b.size() == 1) {
				       s.insert(*b.begin());
				     }
				     else {
				       pair<int, multiset<int> > keyb = make_pair(p->type, b);
				       s.insert(unique[keyb]);
				     }
				     ss.insert(s);
				   }
				   optypes.push_back(p->type);
				   operands.push_back(ss);
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
  operands.clear();
  operands.resize(ndata);
  unique.clear();
  for(auto s : outputnames) {
    opnode * p = data_name2opnode[s];
    if(!p) {
      show_error("output data " + s + " function unspecified");
    }
    gen_operands_opnode(p);
  }
}


void Dfg::support_MAC() {
  assert(optypes.size() == operands.size());
  int n = optypes.size();
  vector<set<set<int> > > new_operands(n);
  for(int i = 0; i < n; i++) {
    if(optypes[i] != optype("+")) {
      continue;
    }
    for(auto s : operands[i]) {
      assert(s.size() == 2);
      auto it = s.begin();
      int a = *it;
      it++;
      int b = *it;
      if(optypes[a] == optype("*")) {
	for(auto cs : operands[a]) {
	  assert(cs.size() == 2);
	  set<int> new_s;
	  for(auto cc : cs) {
	    new_s.insert(cc);
	  }
	  new_s.insert(b);
	  new_operands[i].insert(new_s);
	}
      }
      if(optypes[b] == optype("*")) {
	for(auto cs : operands[b]) {
	  assert(cs.size() == 2);
	  set<int> new_s;
	  for(auto cc : cs) {
	    new_s.insert(cc);
	  }
	  new_s.insert(a);
	  new_operands[i].insert(new_s);
	}
      }      
    }
  }
  for(int i = 0; i < n; i++) {
    for(auto s : new_operands[i]) {
      operands[i].insert(s);
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
