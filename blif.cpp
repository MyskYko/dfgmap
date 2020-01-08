#include <iostream>
#include <fstream>
#include <cassert>

#include "op.hpp"
using namespace std;

void write_add(ofstream &f) {
  f << "00 0" << endl;
}
void write_mul(ofstream &f) {
  f << "11 1" << endl;
}

string write_opnode(ofstream &f, opnode *p, int &id, vector<string> &inputnames) {
  if(p->id != -1) {
    string name;
    if(p->id < inputnames.size()) {
      name = inputnames[p->id];
    }
    else {
      name = "_n" + to_string(p->id);
    }
    return name;
  }
  assert(p->vc.size() == 2);
  assert(p->type);
  string l = write_opnode(f, p->vc[0], id, inputnames);
  string r = write_opnode(f, p->vc[1], id, inputnames);
  p->id = id++;
  string name = "_n" + to_string(p->id);
  f << ".names " << l << " " << r << " " << name << endl;
  if(typeop(p->type) == "+") {
    write_add(f);
  } else if(typeop(p->type) == "*") {
    write_mul(f);
  }
  else {
    show_error("unknown operation type");
  }
  return name;
}

void gen_spec(string specfilename, vector<string> &inputnames, vector<string> &outputnames, vector<opnode *> &outputs) {
  ofstream f(specfilename);
  if(!f) {
    show_error("cannot open spec file");
  }
  f << ".model spec" << endl;
  f << ".inputs";
  for(auto name : inputnames) {
    f << " " << name;
  }
  f << endl;
  f << ".outputs";
  for(auto name : outputnames) {
    f << " " << name;
  }
  f << endl;
  int id = inputnames.size();
  for(int i = 0; i < outputs.size(); i++) {
    auto p = outputs[i];
    string name = write_opnode(f, p, id, inputnames);
    f << ".names " << name << " " << outputnames[i] << endl;
    f << "1 1" << endl;
  }
  f << ".end" << endl;
  return;
}

int gen_tmpl(string tmplfilename, int ncycles, int nregs, int nnodes, int nops, vector<opnode *> &operators, vector<pair<int, int> > &coms, vector<string> &inputnames, vector<string> &outputnames, map<string, vector<pair<int, string> > > &mcand) {
  ofstream f(tmplfilename);
  if(!f) {
    show_error("cannot open tmpl file");
  }
  f << ".model tmpl" << endl;
  f << ".inputs";
  for(auto name : inputnames) {
    f << " " << name;
  }
  f << endl;
  f << ".outputs";
  for(auto name : outputnames) {
    f << " " << name;
  }
  f << endl;

  vector<vector<int> > recv(nnodes);
  for(int i = 0; i < coms.size(); i++) {
    auto com = coms[i];
    recv[com.second].push_back(i);
  }
  
  int nsels = 0;
  int maxsel = 0;
  for(int t = 0; t < ncycles; t++) {
    for(int n = 0; n < nnodes; n++) {
      // register
      for(int r = 0; r < nregs; r++) {
	string name = "reg_t" + to_string(t) + "n" + to_string(n) + "r" + to_string(r);
	// first cycle
	if(t == 0) {
	  f << ".subckt mux";
	  f << " k=" << name;
	  vector<pair<int, string> > vcand;
	  int i = 0;
	  for(; i < inputnames.size();) {
	    f << " i" << i << "=" << inputnames[i];
	    f << " j" << i << "=s" << nsels;
	    vcand.push_back(make_pair(nsels, inputnames[i]));
	    i++;
	    nsels++;
	  }
	  mcand[name] = vcand;
	  f << endl;
	  if(i > maxsel) {
	    maxsel = i;
	  }
	}
	// the subsequent cycles
	else {
	  f << ".subckt mux";
	  f << " k=" << name;
	  vector<pair<int, string> > vcand;
	  int i = 0;
	  string s;
	  for(; i < nregs;) {
	    s = "reg_t" + to_string(t-1) + "n" + to_string(n) + "r" + to_string(i);
	    f << " i" << i << "=" << s;
	    f << " j" << i << "=s" << nsels;
	    vcand.push_back(make_pair(nsels, s));
	    i++;
	    nsels++;
	  }
	  s = "ope_t" + to_string(t-1) + "n" + to_string(n);
	  f << " i" << i << "=" << s;
	  f << " j" << i << "=s" << nsels;
	  vcand.push_back(make_pair(nsels, s));
	  i++;
	  nsels++;
	  for(auto p : recv[n]) {
	    s = "com_t" + to_string(t-1) + "p" + to_string(p);
	    f << " i" << i << "=" << s;
	    f << " j" << i << "=s" << nsels;
	    vcand.push_back(make_pair(nsels, s));
	    i++;
	    nsels++;
	  }
	  mcand[name] = vcand;
	  f << endl;
	  if(i > maxsel) {
	    maxsel = i;
	  }
	}
      }
      // operation
      string name = "ope_t" + to_string(t) + "n" + to_string(n);
      string s;
      for(int i = 0; i < operators.size(); i++) {
	s = name + "o" + to_string(i);
	f << ".subckt ope" << i;
	f << " out=" << s;
	for(int j = 0; j < nops; j++) {
	  char c = 'a' + j;
	  string r = "reg_t" + to_string(t) + "n" + to_string(n) + "r" + to_string(j);
	  f << " " << c << "=" << r;
	}
	f << endl;
      }
      f << ".subckt mux";
      f << " k=" << name;
      vector<pair<int, string> > vcand;
      int i = 0;
      for(; i < operators.size();) {
	s = name + "o" + to_string(i);
	f << " i" << i << "=" << s;
	f << " j" << i << "=s" << nsels;
	vcand.push_back(make_pair(nsels, s));
	i++;
	nsels++;
      }
      mcand[name] = vcand;
      f << endl;
      if(i > maxsel) {
	maxsel = i;
      }
    }
    if(t == ncycles-1) {
      break;
    }
    // communication
    for(int p = 0; p < coms.size(); p++) {
      string name = "com_t" + to_string(t) + "p" + to_string(p);
      int n = coms[p].first;
      f << ".subckt mux";
      f << " k=" << name;
      vector<pair<int, string> > vcand;
      int i = 0;
      string s;
      for(; i < nregs;) {
	s = "reg_t" + to_string(t) + "n" + to_string(n) + "r" + to_string(i);
	f << " i" << i << "=" << s;
	f << " j" << i << "=s" << nsels;
	vcand.push_back(make_pair(nsels, s));
	i++;
	nsels++;
      }
      s = "ope_t" + to_string(t) + "n" + to_string(n);
      f << " i" << i << "=" << s;
      f << " j" << i << "=s" << nsels;
      vcand.push_back(make_pair(nsels, s));
      i++;
      nsels++;
      mcand[name] = vcand;
      f << endl;
      if(i > maxsel) {
	maxsel = i;
      }
    }
  }
  // outputs
  for(auto name : outputnames) {
    f << ".subckt mux";
    f << " k=" << name;
    vector<pair<int, string> > vcand;
    int i = 0;
    string s;
    int t = ncycles-1;
    for(int n = 0; n < nnodes; n++) {
      for(int r = 0; r < nregs; r++) {
	s = "reg_t" + to_string(t) + "n" + to_string(n) + "r" + to_string(r);
	f << " i" << i << "=" << s;
	f << " j" << i << "=s" << nsels;
	vcand.push_back(make_pair(nsels, s));
	i++;
	nsels++;
      }
      s = "ope_t" + to_string(t) + "n" + to_string(n);
      f << " i" << i << "=" << s;
      f << " j" << i << "=s" << nsels;
      vcand.push_back(make_pair(nsels, s));
      i++;
      nsels++;
    }
    mcand[name] = vcand;
    f << endl;
    if(i > maxsel) {
      maxsel = i;
    }
  }
  // add select signals to inputs
  f << ".inputs";
  for(int i = 0; i < nsels; i++) {
    f << " s" << i;
  }
  f << endl;
  f << ".end" << endl;
  // mux
  f << ".model mux" << endl;
  f << ".inputs";
  for(int i = 0; i < maxsel; i++) {
    f << " i" << i;
    f << " j" << i;
  }
  f << endl;
  f << ".outputs k" << endl;
  f << ".names";
  for(int i = 0; i < maxsel; i++) {
    f << " i" << i;
    f << " j" << i;
  }
  f << " k" << endl;
  for(int i = 0; i < maxsel; i++) {
    for(int j = 0; j < maxsel; j++) {
      if(i == j) {
	f << "11";
      }
      else {
	f << "--";
      }
    }
    f << " 1" << endl;
  }
  f << ".end" << endl;
  // operators
  for(int i = 0; i < operators.size(); i++) {
    f << ".model ope" << i << endl;
    f << ".inputs";
    vector<string> opnames;
    for(int j = 0; j < nops; j++) {
      char c = 'a' + j;
      f << " " << c;
      string s{c};
      opnames.push_back(s);
    }
    f << endl;
    f << ".outputs out" << endl;
    auto p = operators[i];
    int id = nops;
    string name = write_opnode(f, p, id, opnames);
    f << ".names " << name << " out" << endl;
    f << "1 1" << endl;
    f << ".end" << endl;
  }
  return nsels;
}

void gen_top(string topfilename, string specfilename, string tmplfilename, int nsels, vector<string> &inputnames, vector<string> &outputnames, map<string, vector<pair<int, string> > > &mcand) {
  ofstream f(topfilename);
  if(!f) {
    show_error("cannot open top file");    
  }
  f << ".model top" << endl;
  f << ".inputs";
  for(int i = 0; i < nsels; i++) {
    f << " s" << i;
  }
  for(auto name : inputnames) {
    f << " " << name;
  }
  f << endl;
  f << ".outputs out" << endl;
  // spec
  f << ".subckt spec";
  for(auto name : inputnames) {
    f << " " << name << "=" << name;
  }
  for(auto name : outputnames) {
    f << " " << name << "=_spec_" << name;
  }
  f << endl;
  // tmpl
  f << ".subckt tmpl";
  for(auto name : inputnames) {
    f << " " << name << "=" << name;
  }
  for(int i = 0; i < nsels; i++) {
    f << " s" << i << "=s" << i;
  }
  for(auto name : outputnames) {
    f << " " << name << "=_tmpl_" << name;
  }
  f << endl;
  // eq
  for(auto name : outputnames) {
    f << ".names";
    f << " _spec_" << name;
    f << " _tmpl_" << name;
    f << " _eq_" << name;;
    f << endl;
    f << "11 1" << endl;
    f << "00 1" << endl;
  }
  f << ".names";
  for(auto name : outputnames) {
    f << " _eq_" << name;
  }
  f << " _eq";
  f << endl;
  for(auto name : outputnames) {
    f << "1";
  }
  f << " 1" << endl;
  // cardinality
  set<set<int> > sscand;
  for(auto vcand : mcand) {
    set<int> scand;
    for(auto cand : vcand.second) {
      scand.insert(cand.first);
    }
    sscand.insert(scand);
  }
  int namos = 0;
  int maxcand = 0;
  for(auto scand : sscand) {
    f << ".subckt amo";
    int i = 0;
    for(auto cand : scand) {
      f << " i" << i++ << "=s" << cand;
    }
    f << " out=_amo" << namos++;
    f << endl;
    if(maxcand < i) {
      maxcand = i;
    }
  }
  f << ".names";
  for(int i = 0; i < namos; i++) {
    f << " _amo" << i;
  }
  f << " _amo";
  f << endl;
  for(int i = 0; i < namos; i++) {
    f << "1";
  }
  f << " 1" << endl;
  // condition
  f << ".names _eq _amo out" << endl;
  f << "11 1" << endl;
  f << ".end" << endl;
  // amo
  f << ".model amo" << endl;
  f << ".inputs";
  for(int i = 0; i < maxcand; i++) {
    f << " i" << i;
  }
  f << endl;
  f << ".outputs out" << endl;
  f << ".names";
  for(int i = 0; i < maxcand; i++) {
    f << " i" << i;
  }
  f << " out" << endl;
  for(int i = 0; i < maxcand+1; i++) {
    for(int j = 0; j < maxcand; j++) {
      if(i == j) {
	f << "1";
      }
      else {
	f << "0";
      }
    }
    f << " 1" << endl;
  }
    
  f << ".end" << endl;
  f.close();
  string cmd = "cat " + specfilename + " " + tmplfilename + " >> " + topfilename;
  system(cmd.c_str());
}

void show_result(int ncycles, int nnodes, int nregs, vector<pair<int, int> > &coms, vector<int> &result, map<string, vector<pair<int, string> > > &mcand, vector<string> &outputnames) {
  for(int t = 0; t < ncycles; t++) {
    for(int n = 0; n < nnodes; n++) {
      for(int r = 0; r < nregs; r++) {
	string name = "reg_t" + to_string(t) + "n" + to_string(n) + "r" + to_string(r);
	cout << name << " = ";
	for(auto cand : mcand[name]) {
	  if(result[cand.first]) {
	    cout << cand.second;
	    break;
	  }
	}
	cout << endl;
      }
      string name = "ope_t" + to_string(t) + "n" + to_string(n);      
      cout << name << " = ";
      for(auto cand : mcand[name]) {
	if(result[cand.first]) {
	  cout << cand.second;
	  break;
	}
      }
      cout << endl;
    }
    if(t == ncycles-1) {
      break;
    }
    for(int p = 0; p < coms.size(); p++) {
      string name = "com_t" + to_string(t) + "p" + to_string(p);    
      cout << name << " = ";
      for(auto cand : mcand[name]) {
	if(result[cand.first]) {
	  cout << cand.second;
	  break;
	}
      }
      cout << endl;
    }
  }
  for(auto name : outputnames) {
    cout << name << " = ";
    for(auto cand : mcand[name]) {
      if(result[cand.first]) {
	cout << cand.second;
	break;
      }
    }
    cout << endl;
  }
}
