#include <functional>
#include <cassert>

#include "op.hpp"

using namespace std;

int optype(string s) {
  switch(s[0]) {
  case '+':
    return 1;
  case '*':
    return 2;
  default:
    break;
  }
  return 0;
}

string typeop(int i) {
  switch(i) {
  case 1:
    return "+";
  case 2:
    return "*";
  default:
    break;
  }
  return "";
}


opnode *create_opnode(vector<string> &vs, int &pos, map<string, opnode *> &data_name2opnode) {
  if(!optype(vs[pos])) {
    opnode *p = data_name2opnode[vs[pos]];
    if(!p) {
      show_error("data " + vs[pos] + " unspecified");
    }
    return p;
  }
  opnode *p = new opnode;
  p->id = -1;
  p->type = optype(vs[pos]);
  opnode *l = create_opnode(vs, ++pos, data_name2opnode);
  p->vc.push_back(l);
  opnode *r = create_opnode(vs, ++pos, data_name2opnode);
  p->vc.push_back(r);
  return p;
}

void print_opnode(opnode *p, int depth) {
  for(int i = 0; i < depth; i++) {
    cout << "\t";
  }
  if(!p->type) {
    cout << p->id << endl;
  } else {
    cout << typeop(p->type) << endl;
  }
  for(auto c : p->vc) {
    print_opnode(c, depth + 1);
  }
}

void compress_opnode(opnode *p) {
  if(!p->type) {
    return;
  }
  vector<opnode *> vcn;
  for(int i = 0; i < p->vc.size(); i++) {
    auto c = p->vc[i];
    if(p->type == c->type) {
      for(auto cc : c->vc) {
	p->vc.push_back(cc);
      }
    } else {
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

void gen_operands(opnode *p, int &ndata, vector<set<set<int> > > &operands, map<pair<int, multiset<int> >, int> &unique, vector<string> &datanames, int fmac) {
  if(p->id != -1) {
    return;
  }
  multiset<int> cids;
  for(auto c : p->vc) {
    gen_operands(c, ndata, operands, unique, datanames, fmac);
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
					 } else {
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
				       } else {
					 s.insert(unique[keya]);
				       }
				       if(b.size() == 1) {
					 s.insert(b[0]->id);
				       } else {
					 s.insert(unique[keyb]);
				       }
				       ss.insert(s);
				       // MAC
				       if(fmac) {
					 if(a.size() == 1 && p->type == optype("+") && a[0]->type == optype("*")) {
					   for(auto cs : operands[a[0]->id]) {
					     assert(cs.size() == 2);
					     s.clear();
					     for(auto cc : cs) {
					       s.insert(cc);
					     }
					     if(b.size() == 1) {
					       s.insert(b[0]->id);
					     } else {
					       s.insert(unique[keyb]);
					     }
					     ss.insert(s);
					   }
					 }
					 if(b.size() == 1 && p->type == optype("+") && b[0]->type == optype("*")) {
					   for(auto cs : operands[b[0]->id]) {
					     assert(cs.size() == 2);
					     s.clear();
					     for(auto cc : cs) {
					       s.insert(cc);
					     }
					     if(a.size() == 1) {
					       s.insert(a[0]->id);
					     } else {
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
				 });
  }
  p->id = unique[key];
}
