#include <algorithm>

#include "sat.hpp"
#include "global.hpp"

using namespace std;

Sat::Sat(vector<int> i_nodes, vector<int> o_nodes, vector<int> pe_nodes, vector<set<int> > cons, int ninputs, set<int> output_ids, vector<set<set<int> > > operands) : i_nodes(i_nodes), o_nodes(o_nodes), pe_nodes(pe_nodes), cons(cons), ninputs(ninputs), output_ids(output_ids), operands(operands)
{
  nnodes = 1 + i_nodes.size() + o_nodes.size() + pe_nodes.size();
  ndata = operands.size();
  freg = 0;
  
  for(int i : i_nodes) {
    if(cons[i].size()) {
      show_error("there is an input node with input");
    }
  }
  for(int i : o_nodes) {
    if(cons[i].empty()) {
      show_error("there is an output node without input");
    }	     
    for(int j : cons[i]) {
      if(find(pe_nodes.begin(), pe_nodes.end(), j) == pe_nodes.end()) {
	show_error("there is an output node connected to not PE node");
      }
    }
  }
  for(int i : pe_nodes) {
    if(cons[i].empty()) {
      show_error("there is a PE node without input");
    }	     
    for(int j : cons[i]) {
      if(find(o_nodes.begin(), o_nodes.end(), j) != o_nodes.end()) {
	show_error("there is a PE node connected to output node");
      }
    }
    if(i + pe_nodes.size() < nnodes) {
      show_error(".pe should be listed after .i and .o");
    }
  }
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

int cardinality_k(Glucose::SimpSolver &S, int i, int n, int k) {
  vector<int> vVars;
  // first level
  vVars.push_back(i);
  S.newVar();
  Glucose::Lit l0 = Glucose::mkLit(S.nVars()-1);
  S.addClause(~l0);
  for(int l = 1; l < k; l++) {
    vVars.push_back(S.nVars()-1);
  }
  // subsequent levels
  for(int j = 1; j < n; j++) {
    // prohibit overflow (sum>k)
    Glucose::Lit lk = Glucose::mkLit(vVars[k-1]);
    Glucose::Lit lx = Glucose::mkLit(i+j);
    S.addClause(~lk, ~lx);
    if(j == n-1) {
      break;
    }
    for(int l = k-1; l > 0; l--) {
      // compute AND of x and l-1 of previous level
      S.newVar();
      Glucose::Lit la = Glucose::mkLit(S.nVars()-1);
      Glucose::Lit lb = Glucose::mkLit(vVars[l-1]);
      S.addClause(~la, lx);
      S.addClause(~la, lb);
      S.addClause(la, ~lx, ~lb);
      // compute OR of it and l of previous level
      S.newVar();
      Glucose::Lit ln = Glucose::mkLit(S.nVars()-1);
      Glucose::Lit ll = Glucose::mkLit(vVars[l]);
      S.addClause(ln, ~la);
      S.addClause(ln, ~ll);
      S.addClause(~ln, la, ll);
      // keep it at l of this level
      vVars[l] = S.nVars()-1;
    }
    // compute OR of x and 0 of previous level
    S.newVar();
    Glucose::Lit ln = Glucose::mkLit(S.nVars()-1);
    Glucose::Lit ll = Glucose::mkLit(vVars[0]);
    S.addClause(ln, ~lx);
    S.addClause(ln, ~ll);
    S.addClause(~ln, lx, ll);
    // keep it at 0 of this level
    vVars[0] = S.nVars()-1;
  }
  return 0;
}

void Sat::gen_cnf(int ncycles) {
  ncycles_ = ncycles;
  
  assert(!S.nVars());
  while(ncycles * nnodes * ndata > S.nVars()) {
    S.newVar();
  }
  
  // init condition
  for(int i = 0; i < nnodes; i++) {
    for(int j = 0; j < ndata; j++) {
      S.addClause(Glucose::mkLit(i*ndata + j, true));
    }
  }

  // conditions for each cycle
  for(int i = 1; i < ncycles; i++) {
    
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
    Glucose::Lit l = Glucose::mkLit((ncycles-1)*nnodes*ndata + k);
    S.addClause(l);
  }
}

void Sat::gen_image() {
  int nnodes_ = nnodes;
  if(freg) {
    nnodes_ += pe_nodes.size();
  }
  image.resize(ncycles_, vector<vector<int> >(nnodes));
  for(int i = 0; i < ncycles_; i++) {
    for(int j = 0; j < nnodes; j++) {
      for(int k = 0; k < ndata; k++) {
	if(S.model[i*nnodes_*ndata + j*ndata + k] == l_True) {
	  image[i][j].push_back(k);
	}
      }
    }
    if(freg) {
      for(int j = nnodes; j < nnodes_; j++) {
	for(int k = 0; k < ndata; k++) {
	  if(S.model[i*nnodes_*ndata + j*ndata + k] == l_True) {
	    image[i][j-pe_nodes.size()].push_back(k);
	  }
	}
      }
    }
  }
}

void Sat::gen_cnf_exmem(int ncycles) {
  ncycles_ = ncycles;
  
  assert(!S.nVars());
  while(ncycles * nnodes * ndata > S.nVars()) {
    S.newVar();
  }
  
  // init condition
  for(int j = 0; j < ninputs; j++) {
    S.addClause(Glucose::mkLit(j));
  }
  for(int j = ninputs; j < ndata; j++) {
    S.addClause(Glucose::mkLit(j, true));
  }
  for(int i = 1; i < nnodes; i++) {
    for(int j = 0; j < ndata; j++) {
      S.addClause(Glucose::mkLit(i*ndata + j, true));
    }
  }

  // conditions for each cycle
  for(int i = 1; i < ncycles; i++) {

    // conditions for input nodes
    for(int j : i_nodes) {
      cardinality(S, i*nnodes*ndata + j*ndata, ndata);
      for(int k = 0; k < ndata; k++) {
	Glucose::Lit l = Glucose::mkLit(i*nnodes*ndata + j*ndata + k);
	Glucose::Lit lf = Glucose::mkLit((i-1)*nnodes*ndata + k);
	S.addClause(~l, lf);
      }
    }
    
    // conditions for output nodes
    for(int j : o_nodes) {
      cardinality(S, i*nnodes*ndata + j*ndata, ndata);
      for(int k = 0; k < ndata; k++) {
	Glucose::Lit l = Glucose::mkLit(i*nnodes*ndata + j*ndata + k);
	Glucose::vec<Glucose::Lit> ls;
	ls.push(~l);
	for(int f : cons[j]) {
	  Glucose::Lit lf = Glucose::mkLit((i-1)*nnodes*ndata + f*ndata + k);
	  ls.push(lf);
	}
	S.addClause(ls);
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
    Glucose::Lit l = Glucose::mkLit((ncycles-1)*nnodes*ndata + k);
    S.addClause(l);
  }
}

void Sat::gen_cnf_reg(int ncycles, int nregs) {
  ncycles_ = ncycles;
  freg = 1;
  
  int npes = pe_nodes.size();
  int nnodes_ = nnodes + npes;
  
  assert(!S.nVars());
  while(ncycles * nnodes_ * ndata > S.nVars()) {
    S.newVar();
  }
  
  // init condition
  for(int i = 0; i < nnodes_; i++) {
    for(int j = 0; j < ndata; j++) {
      S.addClause(Glucose::mkLit(i*ndata + j, true));
    }
  }

  // conditions for each cycle
  for(int i = 1; i < ncycles; i++) {
    
    // conditions for input nodes
    for(int j : i_nodes) {
      cardinality(S, i*nnodes_*ndata + j*ndata, ninputs);
      for(int k = ninputs; k < ndata; k++) {
	S.addClause(Glucose::mkLit(i*nnodes_*ndata + j*ndata + k, true));
      }
    }
    
    // conditions for output nodes
    for(int j : o_nodes) {
      cardinality_set(S, i*nnodes_*ndata + j*ndata, output_ids);
      for(int k : output_ids) {
	Glucose::Lit l = Glucose::mkLit(i*nnodes_*ndata + j*ndata + k);
	Glucose::vec<Glucose::Lit> ls;
	ls.push(~l);
	for(int f : cons[j]) {
	  Glucose::Lit lf = Glucose::mkLit((i-1)*nnodes_*ndata + f*ndata + k);
	  ls.push(lf);
	  Glucose::Lit lfr = Glucose::mkLit((i-1)*nnodes_*ndata + (f+npes)*ndata + k);
	  ls.push(lfr);
	}
	S.addClause(ls);
      }
      for(int k = 0; k < ndata; k++) {
	if(find(output_ids.begin(), output_ids.end(), k) == output_ids.end()) {	
	  S.addClause(Glucose::mkLit(i*nnodes_*ndata + j*ndata + k, true));
	}
      }
    }
    
    // conditions for external memory
    for(int k = 0; k < ndata; k++) {
      Glucose::Lit l = Glucose::mkLit(i*nnodes_*ndata + k);
      Glucose::vec<Glucose::Lit> ls;
      ls.push(~l);
      Glucose::Lit lp = Glucose::mkLit((i-1)*nnodes_*ndata + k);
      S.addClause(~lp, l);
      ls.push(lp);
      for(int j : o_nodes) {
	Glucose::Lit lo = Glucose::mkLit(i*nnodes_*ndata + j*ndata + k);
	S.addClause(~lo, l);
	ls.push(lo);
      }
      S.addClause(ls);
    }
    
    // conditions for PE nodes
    for(int j : pe_nodes) {
      cardinality(S, i*nnodes_*ndata + j*ndata, ndata);
      
      // create OR of existence of data among adjacent nodes and itself
      vector<int> vVars;
      for(int k = 0; k < ndata; k++) {
	S.newVar();
	Glucose::Lit l = Glucose::mkLit(S.nVars()-1);
	Glucose::vec<Glucose::Lit> ls;
	ls.push(~l);
	Glucose::Lit lself = Glucose::mkLit((i-1)*nnodes_*ndata + j*ndata + k);
	ls.push(lself);
	S.addClause(~lself, l);
	Glucose::Lit lselfr = Glucose::mkLit((i-1)*nnodes_*ndata + (j+npes)*ndata + k);
	ls.push(lselfr);
	S.addClause(~lselfr, l);
	for(int f : cons[j]) {
	  Glucose::Lit lf = Glucose::mkLit((i-1)*nnodes_*ndata + f*ndata + k);
	  ls.push(lf);
	  S.addClause(~lf, l);
	  if(find(pe_nodes.begin(), pe_nodes.end(), j) != pe_nodes.end()) {	  
	    Glucose::Lit lfr = Glucose::mkLit((i-1)*nnodes_*ndata + (f+npes)*ndata + k);
	    ls.push(lfr);
	    S.addClause(~lfr, l);
	  }
	}
	S.addClause(ls);
	vVars.push_back(S.nVars()-1);
      }
      
      // conditions for communication and operation
      for(int k = 0; k < ndata; k++) {
	Glucose::Lit l = Glucose::mkLit(i*nnodes_*ndata + j*ndata + k);
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

      // registers
      for(int k = 0; k < ndata; k++) {
	if(nregs > 0) {
	  cardinality_k(S, i*nnodes_*ndata + (j+npes)*ndata, ndata, nregs);
	}
	Glucose::Lit l = Glucose::mkLit(i*nnodes_*ndata + (j+npes)*ndata + k);
	Glucose::Lit lk = Glucose::mkLit(vVars[k]);
	S.addClause(~l, lk);
      }
    }
  }

  // conditions for output ready
  for(int k : output_ids) {
    Glucose::Lit l = Glucose::mkLit((ncycles-1)*nnodes_*ndata + k);
    S.addClause(l);
  }
}
