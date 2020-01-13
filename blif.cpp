#include <iostream>
#include <fstream>
#include <sstream>
#include <cassert>

#include "op.hpp"
#include "blif.hpp"
using namespace std;

Blif::Blif(vector<string> inputnames, vector<string> outputnames, map<string, int> nodename2id) : inputnames(inputnames), outputnames(outputnames), nodename2id(nodename2id) {}

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

void Blif::gen_spec(string specfilename, vector<opnode *> &outputs) {
  specfilename_ = specfilename;
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

void Blif::gen_tmpl(string tmplfilename, int ncycles, int nregs, int nnodes, int nops, vector<opnode *> &operators, vector<pair<int, int> > &coms) {
  tmplfilename_ = tmplfilename;
  mcand.clear();
  nsels = 0;
  ncycles_ = ncycles;
  nregs_ = nregs;
  nnodes_ = nnodes;
  nops_ = nops;
  operators_ = operators;
  coms_ = coms;
  
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
      /*
      s = "ope_t" + to_string(t) + "n" + to_string(n);
      f << " i" << i << "=" << s;
      f << " j" << i << "=s" << nsels;
      vcand.push_back(make_pair(nsels, s));
      i++;
      nsels++;
      */
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
}

void Blif::write_constraints(ofstream &f, string pfilename, int &namos, int &maxcand) {
  ifstream pfile(pfilename);
  if(!pfile) {
    cout << "no parameter file" << endl;
    return;
  }
  int nconstraints = 0;
  string str;
  while(getline(pfile, str)) {
    string s;
    stringstream ss(str);
    vector<string> vs;
    while(getline(ss, s, ' ')) {
      vs.push_back(s);
    }
    if(vs.empty()) {
      continue;
    }
    if(vs[0] == ".assign_inputs") {
      while(getline(pfile, str)) {
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
	int t = 0;
	vector<vector<int> > fcand;
	int ncand = 0;
	fcand.resize(nnodes_);
	for(int n = 0; n < nnodes_; n++) {
	  fcand[n].resize(nregs_);
	}
	for(int i = 1; i < vs.size(); i++) {
	  assert(nodename2id.count(vs[i]));
	  string name = vs[i];
	  if(i+1 < vs.size() && vs[i+1] == "{") {
	    i += 2;
	    for(;i < vs.size(); i++) {
	      if(vs[i] == "}") {
		break;
	      }
	      int r;
	      try {
		r = stoi(vs[i]) - 1;
	      } catch(...) {
		show_error("unexpected error at option assign_inputs");
	      }
	      assert(r >= 0 && r < nregs_);
	      fcand[nodename2id[name]][r] = 1;
	      ncand++;
	    }
	  }
	  else {
	    for(int r = 0; r < nregs_; r++) {
	      fcand[nodename2id[name]][r] = 1;
	      ncand++;
	    }
	  }
	}
	for(int n = 0; n < nnodes_; n++) {
	  for(int r = 0; r < nregs_; r++) {
	    string name = "reg_t" + to_string(t) + "n" + to_string(n) + "r" + to_string(r);
	    if(fcand[n][r]) {
	      if(ncand == 1) {
		for(auto cand : mcand[name]) {
		  if(cand.second == vs[0]) {
		    f << ".names s" << cand.first << endl;
		    f << "1" << endl;
		    fsels[cand.first] = -2;
		    nconstraints++;
		  }
		}
	      }
	      continue;
	    }
	    for(auto cand : mcand[name]) {
	      if(cand.second == vs[0]) {
		fsels[cand.first] = -1;
		nconstraints++;
	      }
	    }
	  }
	}
      }
    }
    if(vs[0] == ".assign_outputs") {
      while(getline(pfile, str)) {
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
	vector<vector<int> > fcand;
	int ncand = 0;
	fcand.resize(nnodes_);
	for(int n = 0; n < nnodes_; n++) {
	  fcand[n].resize(nregs_+1);
	}
	for(int i = 1; i < vs.size(); i++) {
	  assert(nodename2id.count(vs[i]));
	  string name = vs[i];
	  if(i+1 < vs.size() && vs[i+1] == "{") {
	    i += 2;
	    for(;i < vs.size(); i++) {
	      if(vs[i] == "}") {
		break;
	      }
	      int r;
	      try {
		r = stoi(vs[i]) - 1;
	      } catch(...) {
		show_error("unexpected error at option assign_inputs");
	      }
	      if(r == -1) {
		r = nregs_;
	      }
	      assert(r >= 0 && r <= nregs_);
	      fcand[nodename2id[name]][r] = 1;
	      ncand++;
	    }
	  }
	  else {
	    for(int r = 0; r < nregs_+1; r++) {
	      fcand[nodename2id[name]][r] = 1;
	      ncand++;
	    }
	  }
	}
	assert(mcand.count(vs[0]));
	for(auto cand : mcand[vs[0]]) {
	  int pos = cand.second.find("n");
	  s = cand.second.substr(pos+1);
	  pos = s.find("r");
	  string sr;
	  if(pos != string::npos) {
	    sr = s.substr(pos+1);
	    s = s.substr(0, pos);
	  }
	  int n;
	  try {
	    n = stoi(s);
	  } catch(...) {
	    show_error("unexpected error at option assign_outputs");
	  }
	  if(sr.empty()) {
	    // ope
	    if(fcand[n][nregs_]) {
	      if(ncand == 1) {
		f << ".names s" << cand.first << endl;
		f << "1" << endl;
		fsels[cand.first] = -2;
		nconstraints++;
	      }
	      continue;
	    }
	    fsels[cand.first] = -1;
	    nconstraints++;
	    continue;
	  }
	  // reg
	  int r;
	  try {
	    r = stoi(sr);
	  } catch(...) {
	    show_error("unexpected error at option assign_outputs");
	  }
	  if(fcand[n][r]) {
	    if(ncand == 1) {
	      f << ".names s" << cand.first << endl;
	      f << "1" << endl;
	      fsels[cand.first] = -2;
	      nconstraints++;
	    }
	    continue;
	  }
	  fsels[cand.first] = -1;
	  nconstraints++;
	}
      }      
    }
    if(vs[0] == ".assign_regs") {
      while(getline(pfile, str)) {
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
	vector<int> fcand;
	int ncand = 0;
	fcand.resize(nregs_+2);
	for(int i = 1; i < vs.size(); i++) {
	  int r;
	  try {
	    r = stoi(vs[i])-1;
	  } catch(...) {
	    show_error("unexpected error at option assign_regs");
	  }
	  if(r == -1) {
	    r = nregs_;
	  }
	  if(r == -2) {
	    r = nregs_+1;
	  }
	  assert(r >= 0 && r <= nregs_+1);
	  fcand[r] = 1;
	  ncand++;
	}
	int r1;
	try {
	  r1 = stoi(vs[0])-1;
	} catch(...) {
	  show_error("unexpected error at option assign_regs");
	}
	assert(r1 >= 0 && r1 < nregs_);
	for(int t = 1; t < ncycles_; t++) {
	  for(int n = 0; n < nnodes_; n++) {
	    string name = "reg_t" + to_string(t) + "n" + to_string(n) + "r" + to_string(r1);
	    for(auto cand : mcand[name]) {
	      if(cand.second.find("ope") != string::npos) {
		// ope
		if(fcand[nregs_]) {
		  if(ncand == 1) {
		    f << ".names s" << cand.first << endl;
		    f << "1" << endl;
		    fsels[cand.first] = -2;
		    nconstraints++;
		  }
		  continue;
		}
		fsels[cand.first] = -1;
		nconstraints++;
		continue;
	      }
	      if(cand.second.find("com") != string::npos) {
		// com
		if(fcand[nregs_+1]) {
		  if(ncand == 1) {
		    f << ".names s" << cand.first << endl;
		    f << "1" << endl;
		    fsels[cand.first] = -2;
		    nconstraints++;
		  }
		  continue;
		}
		fsels[cand.first] = -1;
		nconstraints++;
		continue;
	      }
	      // reg
	      int pos = cand.second.find("n");
	      s = cand.second.substr(pos+1);
	      pos = s.find("r");
	      s = s.substr(pos+1);
	      int r2;
	      try {
		r2 = stoi(s);
	      } catch(...) {
		show_error("unexpected error at option assign_regs");
	      }
	      if(fcand[r2]) {
		if(ncand == 1) {
		  f << ".names s" << cand.first << endl;
		  f << "1" << endl;
		  fsels[cand.first] = -2;
		  nconstraints++;
		}
		continue;
	      }
	      fsels[cand.first] = -1;
	      nconstraints++;
	    }
	  }
	}
      }
    }
    if(vs[0] == ".assign_coms") {
      while(getline(pfile, str)) {
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
	vector<int> fcand;
	int ncand = 0;
	fcand.resize(nregs_+1);
	for(int i = 1; i < vs.size(); i++) {
	  int r;
	  try {
	    r = stoi(vs[i])-1;
	  } catch(...) {
	    show_error("unexpected error at option assign_coms");
	  }
	  if(r == -1) {
	    r = nregs_;
	  }
	  assert(r >= 0 && r <= nregs_);
	  fcand[r] = 1;
	  ncand++;
	}
	int p;
	try {
	  p = stoi(vs[0])-1;
	} catch(...) {
	  show_error("unexpected error at option assign_coms");
	}
	assert(p >= 0 && p < coms_.size());
	for(int t = 0; t < ncycles_-1; t++) {
	  string name = "com_t" + to_string(t) + "p" + to_string(p);
	  for(auto cand : mcand[name]) {
	    int pos = cand.second.find("n");
	    s = cand.second.substr(pos+1);
	    pos = s.find("r");
	    s = s.substr(pos+1);
	    int r;
	    try {
	      r = stoi(s);
	    } catch(...) {
	      show_error("unexpected error at option assign_coms");
	    }
	    if(fcand[r]) {
	      if(ncand == 1) {
		f << ".names s" << cand.first << endl;
		f << "1" << endl;
		fsels[cand.first] = -2;
		nconstraints++;
	      }
	      continue;
	    }
	    fsels[cand.first] = -1;
	    nconstraints++;
	  }
	}
      }
    }
    if(vs[0] == ".amo") {
      for(int i = 1; i < vs.size(); i++) {
	vector<int> scand;
	int t = 0;
	for(int n = 0; n < nnodes_; n++) {
	  for(int r = 0; r < nregs_; r++) {
	    string name = "reg_t" + to_string(t) + "n" + to_string(n) + "r" + to_string(r);
	    for(auto cand : mcand[name]) {
	      if(cand.second == vs[i]) {
		scand.push_back(cand.first);
	      }
	    }
	  }
	}
	f << ".subckt amo";
	int j = 0;
	for(auto cand : scand) {
	  f << " i" << j++ << "=s" << cand;
	}
	f << " out=_amo" << namos++;
	f << endl;
	if(maxcand < j) {
	  maxcand = j;
	}
      }
    }
    if(vs[0] == ".fixop") {
      for(int t = 0; t < ncycles_; t++) {
	for(int n = 0; n < nnodes_; n++) {
	  string name = "ope_t" + to_string(t) + "n" + to_string(n);
	  assert(mcand[name].size() == 1);
	  for(auto cand : mcand[name]) {
	    f << ".names s" << cand.first << endl;
	    f << "1" << endl;
	    fsels[cand.first] = -2;
	    nconstraints++;
	  }
	}
      }
    }
    if(vs[0] == ".symmetric") {
      for(int t = 1; t < ncycles_; t++) {
	for(int n = 1; n < nnodes_; n++) {
	  for(int r = 0; r < nregs_; r++) {
	    string name0 = "reg_t" + to_string(t) + "n" + to_string(0) + "r" + to_string(r);
	    string name = "reg_t" + to_string(t) + "n" + to_string(n) + "r" + to_string(r);
	    assert(mcand[name0].size() == mcand[name].size());
	    for(int i = 0; i < mcand[name0].size(); i++) {
	      auto cand0 = mcand[name0][i];
	      auto cand = mcand[name][i];
	      assert(fsels[cand0.first] == fsels[cand.first]);
	      if(fsels[cand0.first]) {
		continue;
	      }
	      f << ".names s" << cand0.first << " s" << cand.first << endl;
	      f << "1 1" << endl;
	      fsels[cand.first] = cand0.first + 1;
	      nconstraints++;
	    }
	  }
	}
      }
    }
    if(vs[0] == ".repeat") {
      while(getline(pfile, str)) {
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
	for(int j = 1; j < vs.size(); j++) {
	  int t0, t;
	  try {
	    t0 = stoi(vs[0])-1;
	    t = stoi(vs[j])-1;
	  } catch (...) {
	    show_error("unexpected error at option repeat");
	  }
	  for(int n = 0; n < nnodes_; n++) {
	    for(int r = 0; r < nregs_; r++) {
	      string name0 = "reg_t" + to_string(t0) + "n" + to_string(n) + "r" + to_string(r);
	      string name = "reg_t" + to_string(t) + "n" + to_string(n) + "r" + to_string(r);
	      assert(mcand[name0].size() == mcand[name].size());
	      for(int i = 0; i < mcand[name0].size(); i++) {
		auto cand0 = mcand[name0][i];
		auto cand = mcand[name][i];
		if(fsels[cand0.first] < 0) {
		  assert(fsels[cand0.first] == fsels[cand.first]);
		}
		if(fsels[cand0.first] > 0) {
		  assert(fsels[cand.first] > 0);
		  int id0 = fsels[cand0.first];
		  while(fsels[id0-1] > 0) {
		    id0 = fsels[id0-1];
		  }
		  int id = fsels[cand.first];
		  while(fsels[id-1] > 0) {
		    id = fsels[id-1];
		  }
		  assert(id0 == id);
		}
		if(fsels[cand0.first]) {
		  continue;
		}
		f << ".names s" << cand0.first << " s" << cand.first << endl;
		f << "1 1" << endl;
		fsels[cand.first] = cand0.first + 1;
		nconstraints++;
	      }
	    }
	  }
	}
      }
    }
  }

  nsels_ = nsels - nconstraints;
  int count = 0;
  for(int i = 0; i < nsels; i++) {
    if(fsels[i]) {
      count++;
    }
  }
  assert(count == nconstraints);
}

void Blif::gen_top(string topfilename, string pfilename) {
  topfilename_ = topfilename;
  ofstream f(topfilename);
  if(!f) {
    show_error("cannot open top file");    
  }
  f << ".model top" << endl;
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
  // options
  fsels.clear();
  fsels.resize(nsels);
  write_constraints(f, pfilename, namos, maxcand);
  f << ".inputs";
  for(int i = 0; i < nsels; i++) {
    if(fsels[i]) {
      continue;
    }
    f << " s" << i;
  }
  for(auto name : inputnames) {
    f << " " << name;
  }
  f << endl;
  // condition
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
  string cmd = "cat " + specfilename_ + " " + tmplfilename_ + " >> " + topfilename;
  system(cmd.c_str());
}

int Blif::synthesize(string logfilename) {
  result.clear();
  
  string cmd = "abc -c \"read " + topfilename_ + "; strash; qbf -v -P " + to_string(nsels_) + "\" > " + logfilename;
  system(cmd.c_str());

  ifstream lfile(logfilename);
  if(!lfile) {
    show_error("cannot open log file");
  }
  string str;
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
    return 0;
  }
  vector<int> result_;
  int j = 0;
  for(int i = 0; i < nsels; i++) {
    if(fsels[i] == -1) {
      result_.push_back(0);
    }
    else if(fsels[i] == -2) {
      result_.push_back(1);
    }
    else if(fsels[i] > 0) {
      result_.push_back(0);
    }
    else {
      result_.push_back(result[j++]);
    }
  }
  for(int i = 0; i < nsels; i++) {
    if(fsels[i] > 0) {
      int id = fsels[i];
      while(fsels[id-1] > 0) {
	id = fsels[id-1];
      }
      result_[i] = result_[id-1];
    }
  }
  assert(j == nsels_);
  result = result_;
  return 1;
}

void Blif::show_result() {
  for(int t = 0; t < ncycles_; t++) {
    for(int n = 0; n < nnodes_; n++) {
      for(int r = 0; r < nregs_; r++) {
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
    if(t == ncycles_-1) {
      break;
    }
    for(int p = 0; p < coms_.size(); p++) {
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

void Blif::gen_image() {
  image.resize(ncycles_);
  for(int t = 0; t < ncycles_; t++) {
    image[t].resize(nnodes_ + coms_.size());
    for(int n = 0; n < nnodes_; n++) {
      image[t][n].resize(nregs_+1);
    }
    for(int n = nnodes_; n < nnodes_+coms_.size(); n++) {
      image[t][n].resize(1);
    }
  }
  
  map<string, string> pos2data;
  
  for(int t = 0; t < ncycles_; t++) {
    for(int n = 0; n < nnodes_; n++) {
      for(int r = 0; r < nregs_; r++) {
	string name = "reg_t" + to_string(t) + "n" + to_string(n) + "r" + to_string(r);
	for(auto cand : mcand[name]) {
	  if(result[cand.first]) {
	    string s = cand.second;
	    if(!pos2data[s].empty()) {
	      s = pos2data[s];
	    }
	    pos2data[name] = s;
	    break;
	  }
	}
	if(!pos2data.count(name)) {
	  pos2data[name] = "0";
	}
	image[t][n][r] = pos2data[name];
      }
      string name = "ope_t" + to_string(t) + "n" + to_string(n);      
      for(auto cand : mcand[name]) {
	if(result[cand.first]) {
	  int pos = cand.second.rfind("o");
	  string str = cand.second.substr(pos+1);
	  int o;
	  try {
	    o = stoi(str);
	  } catch(...) {
	    show_error("unexpected error at operator in result");
	  }
	  vector<string> opnames;
	  for(int i = 0; i < nops_; i++) {
	    string s = "reg_t" + to_string(t) + "n" + to_string(n) + "r" + to_string(i);
	    s = pos2data[s];
	    opnames.push_back(s);
	  }
	  string s = gen_opstr(operators_[o], opnames);
	  pos2data[name] = s;
	  break;
	}
      }
      if(!pos2data.count(name)) {
	pos2data[name] = "0";
      }
      image[t][n][nregs_] = pos2data[name];
    }
    if(t == ncycles_-1) {
      break;
    }
    for(int p = 0; p < coms_.size(); p++) {
      string name = "com_t" + to_string(t) + "p" + to_string(p);    
      for(auto cand : mcand[name]) {
	if(result[cand.first]) {
	  string s = cand.second;
	  if(!pos2data[s].empty()) {
	    s = pos2data[s];
	  }
	  pos2data[name] = s;
	  break;
	}
      }
      if(!pos2data.count(name)) {
	pos2data[name] = "0";
      }
      image[t][nnodes_+p][0] = pos2data[name];
    }
  }
}
