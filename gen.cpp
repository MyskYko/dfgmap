#include <algorithm>
#include <fstream>
#include <sstream>
#include <map>

#include "gen.hpp"
#include "global.hpp"

using namespace std;

Gen::Gen(vector<int> i_nodes, vector<int> o_nodes, vector<int> pe_nodes, vector<set<int> > cons, int ninputs, set<int> output_ids, vector<set<set<int> > > operands) : i_nodes(i_nodes), o_nodes(o_nodes), pe_nodes(pe_nodes), cons(cons), ninputs(ninputs), output_ids(output_ids), operands(operands)
{
  nnodes = 1 + i_nodes.size() + o_nodes.size() + pe_nodes.size();
  ndata = operands.size();
  freg = 0;
  fexmem_ = 0;
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

int cardinality_am1(int &nvars, int &nclauses, vector<int> vVars, ofstream &fcnf) {
  while(vVars.size() > 1) {
    int k = 0;
    for(int j = 0; j < vVars.size()/2; j++) {
      nvars++;
      fcnf << "-" << vVars[2*j] << " ";
      fcnf << "-" << vVars[2*j+1] << " ";
      fcnf << 0 << endl;
      nclauses++;
      fcnf << "-" << nvars << " ";
      fcnf << vVars[2*j] << " ";
      fcnf << vVars[2*j+1] << " ";
      fcnf << 0 << endl;
      nclauses++;
      fcnf << nvars << " ";
      fcnf << "-" << vVars[2*j] << " ";
      fcnf << 0 << endl;
      nclauses++;
      fcnf << nvars << " ";
      fcnf << "-" << vVars[2*j+1] << " ";
      fcnf << 0 << endl;
      nclauses++;
      vVars[k++] = nvars;
    }
    if(vVars.size()%2) {
      vVars[k++] = vVars.back();
    }
    vVars.resize(k);
  }
  return 0;
}

int cardinality_amk(int &nvars, int &nclauses, vector<int> vVars, ofstream &fcnf, int k) {
  vector<int> vCounts;
  // first level
  vCounts.push_back(vVars[0]);
  nvars++;
  fcnf << "-" << nvars << " ";
  fcnf << 0 << endl;
  nclauses++;
  for(int i = 1; i < k; i++) {
    vCounts.push_back(nvars);
  }
  // subsequent levels
  for(int j = 1; j < vVars.size(); j++) {
    int x = vVars[j];
    // prohibit overflow (sum>k)
    fcnf << "-" << vCounts[k-1] << " ";
    fcnf << "-" << x << " ";
    fcnf << 0 << endl;
    nclauses++;
    if(j == vVars.size()-1) {
      break;
    }
    for(int i = k-1; i > 0; i--) {
      // compute AND of x and l-1 of previous level
      nvars++;
      int a = nvars;
      fcnf << "-" << a << " ";
      fcnf << x << " ";
      fcnf << 0 << endl;
      nclauses++;
      fcnf << "-" << a << " ";
      fcnf << vCounts[i-1] << " ";
      fcnf << 0 << endl;
      nclauses++;
      fcnf << a << " ";
      fcnf << "-" << x << " ";
      fcnf << "-" << vCounts[i-1] << " ";
      fcnf << 0 << endl;
      nclauses++;
      // compute OR of it and l of previous level
      nvars++;
      fcnf << "-" << a << " ";
      fcnf << nvars << " ";
      fcnf << 0 << endl;
      nclauses++;
      fcnf << "-" << vCounts[i] << " ";
      fcnf << nvars << " ";
      fcnf << 0 << endl;
      nclauses++;
      fcnf << "-" << nvars << " ";
      fcnf << a << " ";
      fcnf << vCounts[i] << " ";
      fcnf << 0 << endl;
      nclauses++;
      // keep it at l of this level
      vCounts[i] = nvars;
    }
    // compute OR of x and 0 of previous level
    nvars++;
    fcnf << "-" << x << " ";
    fcnf << nvars << " ";
    fcnf << 0 << endl;
    nclauses++;
    fcnf << "-" << vCounts[0] << " ";
    fcnf << nvars << " ";
    fcnf << 0 << endl;
    nclauses++;
    fcnf << "-" << nvars << " ";
    fcnf << x << " ";
    fcnf << vCounts[0] << " ";
    fcnf << 0 << endl;
    nclauses++;
    // keep it at 0 of this level
    vCounts[0] = nvars;
  }
  return 0;
}

void Gen::gen_image(string rfilename) {
  vector<int> results;
  ifstream rfile(rfilename);
  if(!rfile) {
    show_error("cannot open result file");
  }
  string str;
  while(getline(rfile, str)) {
    string s;
    stringstream ss(str);
    vector<string> vs;
    getline(ss, s, ' ');
    if(s == "1" || s == "-1") {
      results.push_back(stoi(s));
      while(getline(ss, s, ' ')) {
	try {
	  results.push_back(stoi(s));
	} catch(...) {
	  show_error("wrong formatted result file");
	}
      }
    } else if(s == "v") {
      while(getline(ss, s, ' ')) {
	try {
	  results.push_back(stoi(s));
	} catch(...) {
	  show_error("wrong formatted result file");
	}
      }
    }
  }
  
  image.resize(ncycles_, vector<vector<int> >(nnodes));
  for(int i = 0; i < ncycles_; i++) {
    for(int j = 0; j < nnodes; j++) {
      for(int k = 0; k < ndata; k++) {
	if(results[i*nnodes_*ndata + j*ndata + k] > 0) {
	  image[i][j].push_back(k);
	}
      }
    }
    for(int j = nnodes; j < nnodes_; j++) {
      for(int k = 0; k < ndata; k++) {
	if(results[i*nnodes_*ndata + j*ndata + k] > 0) {
	  image[i][j-pe_nodes.size()].push_back(k);
	}
      }
    }
  }
}

void Gen::gen_image_ilp(string sfilename) {
  map<string, int> results;
  ifstream sfile(sfilename);
  if(!sfile) {
    show_error("cannot open result file");
  }
  string str;
  while(getline(sfile, str)) {
    string s;
    stringstream ss(str);
    vector<string> vs;
    getline(ss, s, ' ');
    getline(ss, s, ' ');
    getline(ss, s, ' ');
    if(s == "<variable") {
      try {
	getline(ss, s, ' ');
	string dname = s.substr(6);
	dname = dname.substr(0, dname.size()-1);
	getline(ss, s, ' ');
	getline(ss, s, ' ');
	string vname = s.substr(7,s.size()-5);
	results[dname] = stoi(vname);
      } catch(...) {
	show_error("wrong formatted result file");
      }
    }
  }
  
  image.resize(ncycles_, vector<vector<int> >(nnodes));
  for(int i = 0; i < ncycles_; i++) {
    for(int j = 0; j < nnodes; j++) {
      for(int k = 0; k < ndata; k++) {
	string dname = "d(" + to_string(i) + "," + to_string(j) + "," + to_string(k) + ")";
	if(results[dname]) {
	  image[i][j].push_back(k);
	}
      }
    }
    for(int j = nnodes; j < nnodes_; j++) {
      for(int k = 0; k < ndata; k++) {
	string dname = "d(" + to_string(i) + "," + to_string(j) + "," + to_string(k) + ")";
	if(results[dname]) {
	  image[i][j-pe_nodes.size()].push_back(k);
	}
      }
    }
  }
}


void Gen::gen_cnf(int ncycles, int nregs, int fexmem, string cnfname) {
  ncycles_ = ncycles;
  nnodes_ = nnodes;
  fexmem_ = fexmem;
  int npes = pe_nodes.size();
  if(nregs) {
    freg = 1;
    nnodes_ += npes;
  }
  
  ofstream fcnf(cnfname);
  int nvars = ncycles * nnodes_ * ndata;
  int nclauses = 0;
  
  // init condition
  for(int j = 0; j < ninputs; j++) {
    if(fexmem) {
      fcnf << j + 1 << " ";
    } else {
      fcnf << "-" << j + 1 << " ";
    }
    fcnf << 0 << endl;
    nclauses++;
  }
  for(int j = ninputs; j < ndata; j++) {
    fcnf << "-" << j + 1 << " ";
    fcnf << 0 << endl; 
    nclauses++;
  }
  for(int i = 1; i < nnodes_; i++) {
    for(int j = 0; j < ndata; j++) {
      fcnf << "-" << i*ndata + j + 1 << " ";
      fcnf << 0 << endl; 
      nclauses++;
    }
  }

  // conditions for each cycle
  for(int i = 1; i < ncycles; i++) {
    // conditions for input nodes
    for(int j : i_nodes) {
      if(fexmem) {
	// cardinality
	vector<int> vVars;
	for(int k = 0; k < ndata; k++) {
	  vVars.push_back(i*nnodes_*ndata + j*ndata + k + 1);
	}
	cardinality_am1(nvars, nclauses, vVars, fcnf);
	// communication possibility
	for(int k = 0; k < ndata; k++) {
	  fcnf << "-" << i*nnodes_*ndata + j*ndata + k + 1 << " ";
	  fcnf << (i-1)*nnodes_*ndata + k + 1 << " ";
	  fcnf << 0 << endl; 
	  nclauses++;
	}
      } else {
	// cardinality for inputs
	vector<int> vVars;
	for(int k = 0; k < ninputs; k++) {
	  vVars.push_back(i*nnodes_*ndata + j*ndata + k + 1);
	}
	cardinality_am1(nvars, nclauses, vVars, fcnf);
	// false the others
	for(int k = ninputs; k < ndata; k++) {
	  fcnf << "-" << i*nnodes_*ndata + j*ndata + k + 1 << " ";
	  fcnf << 0 << endl; 
	  nclauses++;
	}
      }
    }
    
    // conditions for output nodes
    for(int j : o_nodes) {
      if(fexmem) {
	// cardinality
	vector<int> vVars;
	for(int k = 0; k < ndata; k++) {
	  vVars.push_back(i*nnodes_*ndata + j*ndata + k + 1);
	}
	cardinality_am1(nvars, nclauses, vVars, fcnf);
	// communication possibility
	for(int k = 0; k < ndata; k++) {
	  fcnf << "-" << i*nnodes_*ndata + j*ndata + k + 1 << " ";	  
	  for(int f : cons[j]) {
	    fcnf << (i-1)*nnodes_*ndata + f*ndata + k + 1 << " ";
	    if(freg) {
	      fcnf << (i-1)*nnodes_*ndata + (f+npes)*ndata + k + 1 << " ";
	    }
	  }
	  fcnf << 0 << endl; 
	  nclauses++;
	}
      } else {
	// cardinality for outputs
	vector<int> vVars;
	for(int k : output_ids) {
	  vVars.push_back(i*nnodes_*ndata + j*ndata + k + 1);	
	}
	cardinality_am1(nvars, nclauses, vVars, fcnf);
	// communication possibility for outputs
	for(int k : output_ids) {
	  fcnf << "-" << i*nnodes_*ndata + j*ndata + k + 1 << " ";
	  for(int f : cons[j]) {
	    fcnf << (i-1)*nnodes_*ndata + f*ndata + k + 1 << " ";
	    if(freg) {
	      fcnf << (i-1)*nnodes_*ndata + (f+npes)*ndata + k + 1 << " ";
	    }
	  }
	  fcnf << 0 << endl;
	  nclauses++;
	}
	// false the others
	for(int k = 0; k < ndata; k++) {
	  if(find(output_ids.begin(), output_ids.end(), k) == output_ids.end()) {
	    fcnf << "-" << i*nnodes_*ndata + j*ndata + k + 1 << " ";
	    fcnf << 0 << endl;
	    nclauses++;
	  }
	}
      }
    }
    
    // conditions for external memory
    if(fexmem) {
      for(int k = 0; k < ndata; k++) {
	// hold or communication possibility
	fcnf << "-" << i*nnodes_*ndata + k + 1 << " ";
	fcnf << (i-1)*nnodes_*ndata + k + 1 << " ";
	for(int j : o_nodes) {      
	  fcnf << i*nnodes_*ndata + j*ndata + k + 1 << " ";
	}
	// reverse
	fcnf << 0 << endl;
	nclauses++;
	fcnf << "-" << (i-1)*nnodes_*ndata + k + 1 << " ";      
	fcnf << i*nnodes_*ndata + k + 1 << " ";
	fcnf << 0 << endl;
	nclauses++;
	for(int j : o_nodes) {      
	  fcnf << "-" << i*nnodes_*ndata + j*ndata + k + 1 << " ";
	  fcnf << i*nnodes_*ndata + k + 1 << " ";
	  fcnf << 0 << endl;
	  nclauses++;
	}
      }
    } else {
      for(int k : output_ids) {
	// hold or communication possibility
	fcnf << "-" << i*nnodes_*ndata + k + 1 << " ";
	fcnf << (i-1)*nnodes_*ndata + k + 1 << " ";
	for(int j : o_nodes) {      
	  fcnf << i*nnodes_*ndata + j*ndata + k + 1 << " ";
	}
	// reverse
	fcnf << 0 << endl;
	nclauses++;
	fcnf << "-" << (i-1)*nnodes_*ndata + k + 1 << " ";      
	fcnf << i*nnodes_*ndata + k + 1 << " ";
	fcnf << 0 << endl;
	nclauses++;
	for(int j : o_nodes) {      
	  fcnf << "-" << i*nnodes_*ndata + j*ndata + k + 1 << " ";
	  fcnf << i*nnodes_*ndata + k + 1 << " ";
	  fcnf << 0 << endl;
	  nclauses++;
	}
      }
      // false the others
      for(int k = 0; k < ndata; k++) {
	if(find(output_ids.begin(), output_ids.end(), k) == output_ids.end()) {
	  fcnf << "-" << i*nnodes_*ndata + k + 1 << " ";
	  fcnf << 0 << endl;
	  nclauses++;
	}
      }
    }
    
    // conditions for PE nodes
    for(int j : pe_nodes) {
      // cardinality
      vector<int> vVars;
      for(int k = 0; k < ndata; k++) {
	vVars.push_back(i*nnodes_*ndata + j*ndata + k + 1);
      }
      cardinality_am1(nvars, nclauses, vVars, fcnf);
      // create OR of existence of data among adjacent nodes and itself
      vVars.clear();
      for(int k = 0; k < ndata; k++) {
	nvars++;
	// OR -> 1
	fcnf << "-" << nvars << " ";
	fcnf << (i-1)*nnodes_*ndata + j*ndata + k + 1 << " ";
	if(freg) {
	  fcnf << (i-1)*nnodes_*ndata + (j+npes)*ndata + k + 1 << " ";
	}
	for(int f : cons[j]) {
	  fcnf << (i-1)*nnodes_*ndata + f*ndata + k + 1 << " ";
	  if(freg && find(pe_nodes.begin(), pe_nodes.end(), f) != pe_nodes.end()) {
	    fcnf << (i-1)*nnodes_*ndata + (f+npes)*ndata + k + 1 << " ";
	  }
	}
	fcnf << 0 << endl;
	nclauses++;
	// 1 -> OR
	fcnf << "-" << (i-1)*nnodes_*ndata + j*ndata + k + 1 << " ";
	fcnf << nvars << " ";
	fcnf << 0 << endl;
	nclauses++;
	if(freg) {
	  fcnf << "-" << (i-1)*nnodes_*ndata + (j+npes)*ndata + k + 1 << " ";
	  fcnf << nvars << " ";
	  fcnf << 0 << endl;
	  nclauses++;
	}
	for(int f : cons[j]) {
	  fcnf << "-" << (i-1)*nnodes_*ndata + f*ndata + k + 1 << " ";
	  fcnf << nvars << " ";
	  fcnf << 0 << endl;
	  nclauses++;
	  if(freg && find(pe_nodes.begin(), pe_nodes.end(), f) != pe_nodes.end()) {
	    fcnf << "-" << (i-1)*nnodes_*ndata + (f+npes)*ndata + k + 1 << " ";
	    fcnf << nvars << " ";
	    fcnf << 0 << endl;
	    nclauses++;
	  }
	}
	vVars.push_back(nvars);
      }
      // conditions for communication and operation
      for(int k = 0; k < ndata; k++) {
	vector<int> vVars2;
	// communication possibility
	vVars2.push_back(vVars[k]);
	// operation possibility
	for(auto s : operands[k]) {
	  // AND -> 1
	  nvars++;
	  fcnf << nvars << " ";	  
	  for(int o : s) {
	    fcnf << "-" << vVars[o] << " ";
	  }
	  fcnf << 0 << endl;
	  nclauses++;
	  // 1 -> AND
	  for(int o : s) {
	    fcnf << vVars[o] << " ";
	    fcnf << "-" << nvars << " ";
	    fcnf << 0 << endl;
	    nclauses++;
	  }
	  vVars2.push_back(nvars);
	}
	// any possibility is satisfied
	fcnf << "-" << i*nnodes_*ndata + j*ndata + k + 1 << " ";
	for(int v : vVars2) {
	  fcnf << v << " ";
	}
	fcnf << 0 << endl;
	nclauses++;
      }
      // registers
      if(freg) {
	if(nregs > 0) {
	  // cardinality
	  vector<int> vVars2;
	  for(int k = 0; k < ndata; k++) {
	    vVars2.push_back(i*nnodes_*ndata + (j+npes)*ndata + k + 1);
	  }
	  cardinality_amk(nvars, nclauses, vVars2, fcnf, nregs);
	}
	// communication possibility
	for(int k = 0; k < ndata; k++) {
	  fcnf << "-" << i*nnodes_*ndata + (j+npes)*ndata + k + 1 << " ";
	  fcnf << vVars[k] << " ";
	  fcnf << 0 << endl;
	  nclauses++;
	}
      }
    }
  }

  // conditions for output ready
  for(int k : output_ids) {
    fcnf << (ncycles-1)*nnodes_*ndata + k + 1 << " ";
    fcnf << 0 << endl;
    nclauses++;
  }

  fcnf.close();

  string header = "p cnf " + to_string(nvars) + " " + to_string(nclauses);
  string cmd = "sed -i \'1i" + header + "\' " + cnfname;
  system(cmd.c_str());
}

void Gen::gen_ilp(int ncycles, int nregs, int fexmem, string lpname) {
  ncycles_ = ncycles;
  nnodes_ = nnodes;
  fexmem_ = fexmem;
  int npes = pe_nodes.size();
  if(nregs) {
    freg = 1;
    nnodes_ += npes;
  }

  ofstream flp(lpname);

  flp << "maximize" << endl;
  flp << "subject to" << endl;
  int nclauses = 0;
  int nvars = 0;

  // init condition
  for(int j = 0; j < ninputs; j++) {
    flp << "c" << nclauses++ << ": ";
    flp << "d(" << 0 << "," << 0 << "," << j << ") ";
    flp << "= ";
    if(fexmem) {
      flp << 1;
    } else {
      flp << 0;
    }
    flp << endl;
  }
  for(int j = ninputs; j < ndata; j++) {
    flp << "c" << nclauses++ << ": ";
    flp << "d(" << 0 << "," << 0 << "," << j << ") ";
    flp << "= ";
    flp << 0;
    flp << endl;
  }
  for(int i = 1; i < nnodes_; i++) {
    for(int j = 0; j < ndata; j++) {
      flp << "c" << nclauses++ << ": ";
      flp << "d(" << 0 << "," << i << "," << j << ") ";
      flp << "= ";
      flp << 0;
      flp << endl;
    }
  }
  
  // conditions for each cycle
  for(int i = 1; i < ncycles; i++) {
    // conditions for input nodes
    for(int j : i_nodes) {
      if(fexmem) {
	// cardinality
	flp << "c" << nclauses++ << ": ";
	for(int k = 0; k < ndata; k++) {
	  flp << "+ ";
	  flp << "d(" << i << "," << j << "," << k << ") ";
	}
	flp << "<= ";
	flp << 1;
	flp << endl;
	// communication possibility
	for(int k = 0; k < ndata; k++) {
	  flp << "c" << nclauses++ << ": ";
	  flp << "d(" << i-1 << "," << 0 << "," << k << ") ";
	  flp << "- ";
	  flp << "d(" << i << "," << j << "," << k << ") ";
	  flp << ">= ";
	  flp << 0;
	  flp << endl;
	}
      } else {
	// cardiality for inputs
	flp << "c" << nclauses++ << ": ";
	for(int k = 0; k < ninputs; k++) {
	  flp << "+ ";
	  flp << "d(" << i << "," << j << "," << k << ") ";
	}
	flp << "<= ";
	flp << 1;
	flp << endl;
	// zero for the others
	for(int k = ninputs; k < ndata; k++) {
	  flp << "c" << nclauses++ << ": ";
	  flp << "d(" << i << "," << j << "," << k << ") ";
	  flp << "= ";
	  flp << 0;
	  flp << endl;
	}
      }
    }

    // conditions for output nodes
    for(int j : o_nodes) {
      if(fexmem) {
	// cardinality
	flp << "c" << nclauses++ << ": ";
	for(int k = 0; k < ndata; k++) {
	  flp << "+ ";
	  flp << "d(" << i << "," << j << "," << k << ") ";
	}
	flp << "<= ";
	flp << 1;
	flp << endl;
	// communication possibility
	for(int k = 0; k < ndata; k++) {
	  flp << "c" << nclauses++ << ": ";
	  for(int f : cons[j]) {
	    flp << "+ ";
	    flp << "d(" << i-1 << "," << f << "," << k << ") ";
	    if(freg) {
	      flp << "+ ";
	      flp << "d(" << i-1 << "," << f+npes << "," << k << ") ";
	    }
	  }
	  flp << "- ";
	  flp << "d(" << i << "," << j << "," << k << ") ";
	  flp << ">= ";
	  flp << 0;
	  flp << endl;
	}
      } else {
      // cardinality for outputs
	flp << "c" << nclauses++ << ": ";
	for(int k : output_ids) {
	  flp << "+ ";
	  flp << "d(" << i << "," << j << "," << k << ") ";
	}
	flp << "<= ";
	flp << 1;
	flp << endl;
	// communication possibility
	for(int k : output_ids) {
	  flp << "c" << nclauses++ << ": ";
	  for(int f : cons[j]) {
	    flp << "+ ";
	    flp << "d(" << i-1 << "," << f << "," << k << ") ";
	    if(freg) {
	      flp << "+ ";
	      flp << "d(" << i-1 << "," << f+npes << "," << k << ") ";
	    }
	  }
	  flp << "- ";
	  flp << "d(" << i << "," << j << "," << k << ") ";
	  flp << ">= ";
	  flp << 0;
	  flp << endl;
	}
	// zero for the others
	for(int k = 0; k < ndata; k++) {
	  if(find(output_ids.begin(), output_ids.end(), k) == output_ids.end()) {
	    flp << "c" << nclauses++ << ": ";
	    flp << "d(" << i << "," << j << "," << k << ") ";
	    flp << "= ";
	    flp << 0;
	    flp << endl;
	  }
	}
      }
    }

    // conditions for external memory
    if(fexmem) {
      for(int k = 0; k < ndata; k++) {
	// communication possibility
	flp << "c" << nclauses++ << ": ";
	for(int j : o_nodes) {
	  flp << "+ ";
	  flp << "d(" << i << "," << j << "," << k << ") ";
	}
	flp << "+ ";
	flp << "d(" << i-1 << "," << 0 << "," << k << ") ";
	flp << "- ";
	flp << "d(" << i << "," << 0 << "," << k << ") ";
	flp << ">= ";
	flp << 0;
	flp << endl;
	// keep all ... maybe unnecessary
	flp << "c" << nclauses++ << ": ";
	flp << "d(" << i << "," << 0 << "," << k << ") ";
	flp << "- ";
	flp << "d(" << i-1 << "," << 0 << "," << k << ") ";
	flp << ">= ";
	flp << 0;
	flp << endl;
	for(int j : o_nodes) {
	  flp << "c" << nclauses++ << ": ";
	  flp << "d(" << i << "," << 0 << "," << k << ") ";
	  flp << "- ";
	  flp << "d(" << i << "," << j << "," << k << ") ";
	  flp << ">= ";
	  flp << 0;
	  flp << endl;
	}
      }
    } else {
      for(int k : output_ids) {
	// communication possibility
	flp << "c" << nclauses++ << ": ";
	for(int j : o_nodes) {
	  flp << "+ ";
	  flp << "d(" << i << "," << j << "," << k << ") ";
	}
	flp << "+ ";
	flp << "d(" << i-1 << "," << 0 << "," << k << ") ";
	flp << "- ";
	flp << "d(" << i << "," << 0 << "," << k << ") ";
	flp << ">= ";
	flp << 0;
	flp << endl;
	// keep all ... maybe unnecessary
	flp << "c" << nclauses++ << ": ";
	flp << "d(" << i << "," << 0 << "," << k << ") ";
	flp << "- ";
	flp << "d(" << i-1 << "," << 0 << "," << k << ") ";
	flp << ">= ";
	flp << 0;
	flp << endl;
	for(int j : o_nodes) {
	  flp << "c" << nclauses++ << ": ";
	  flp << "d(" << i << "," << 0 << "," << k << ") ";
	  flp << "- ";
	  flp << "d(" << i << "," << j << "," << k << ") ";
	  flp << ">= ";
	  flp << 0;
	  flp << endl;
	}
      }
      // false the others
      for(int k = 0; k < ndata; k++) {
	if(find(output_ids.begin(), output_ids.end(), k) == output_ids.end()) {
	  flp << "c" << nclauses++ << ": ";
	  flp << "d(" << i << "," << 0 << "," << k << ") ";
	  flp << "= ";
	  flp << 0;
	  flp << endl;
	}
      }
    }
    
    // conditions for PE nodes
    for(int j : pe_nodes) {
      // cardiality
      flp << "c" << nclauses++ << ": ";
      for(int k = 0; k < ndata; k++) {
	flp << "+ ";
	flp << "d(" << i << "," << j << "," << k << ") ";
      }
      flp << "<= ";
      flp << 1;
      flp << endl;
      // create OR of existence of data among adjacent nodes and itself
      vector<int> vVars;
      for(int k = 0; k < ndata; k++) {
	// 1 -> OR
	flp << "c" << nclauses++ << ": ";
	flp << "d(" << i-1 << "," << j << "," << k << ") ";
	if(freg) {
	  flp << "+ ";
	  flp << "d(" << i-1 << "," << j+npes << "," << k << ") ";
	}
	for(int f : cons[j]) {
	  flp << "+ ";
	  flp << "d(" << i-1 << "," << f << "," << k << ") ";
	  if(freg && find(pe_nodes.begin(), pe_nodes.end(), f) != pe_nodes.end()) {
	    flp << "+ ";
	    flp << "d(" << i-1 << "," << f+npes << "," << k << ") ";
	  }
	}
	flp << "- ";
	flp << "v(" << nvars << ") ";
	flp << ">= ";
	flp << 0;
	flp << endl;
	// OR -> 1 .. maybe unnecessary
	flp << "c" << nclauses++ << ": ";
	flp << "v(" << nvars << ") ";
	flp << "- ";
	flp << "d(" << i-1 << "," << j << "," << k << ") ";
	flp << ">= ";
	flp << 0;
	flp << endl;
	if(freg) {
	  flp << "c" << nclauses++ << ": ";
	  flp << "v(" << nvars << ") ";
	  flp << "- ";
	  flp << "d(" << i-1 << "," << j+npes << "," << k << ") ";
	  flp << ">= ";
	  flp << 0;
	  flp << endl;
	}
	for(int f : cons[j]) {
	  flp << "c" << nclauses++ << ": ";
	  flp << "v(" << nvars << ") ";
	  flp << "- ";
	  flp << "d(" << i-1 << "," << f << "," << k << ") ";
	  flp << ">= ";
	  flp << 0;
	  flp << endl;
	  if(freg && find(pe_nodes.begin(), pe_nodes.end(), f) != pe_nodes.end()) {
	    flp << "c" << nclauses++ << ": ";
	    flp << "v(" << nvars << ") ";
	    flp << "- ";
	    flp << "d(" << i-1 << "," << f+npes << "," << k << ") ";
	    flp << ">= ";
	    flp << 0;
	    flp << endl;
	  }
	}
	vVars.push_back(nvars++);
      }
      // conditions for communication and operation
      for(int k = 0; k < ndata; k++) {
	vector<int> vVars2;
	// communication possibility
	vVars2.push_back(vVars[k]);
	// operation possibility
	for(auto s : operands[k]) {
	  // AND -> 1
	  flp << "c" << nclauses++ << ": ";
	  flp << "v(" << nvars << ") ";
	  for(int o : s) {
	    flp << "- ";	    
	    flp << "v(" << vVars[o] << ") ";
	  }
	  flp << ">= ";
	  flp << "- ";
	  flp << s.size() - 1;
	  flp << endl;
	  // 1 -> AND
	  for(int o : s) {
	    flp << "c" << nclauses++ << ": ";
	    flp << "v(" << vVars[o] << ") ";
	    flp << "- ";
	    flp << "v(" << nvars << ") ";
	    flp << ">= ";
	    flp << 0;
	    flp << endl;
	  }
	  vVars2.push_back(nvars++);
	}
	// any one possibility is satisfied
	flp << "c" << nclauses++ << ": ";
	for(int v : vVars2) {
	  flp << "+ ";	    
	  flp << "v(" << v << ") ";
	}
	flp << "- ";
	flp << "d(" << i << "," << j << "," << k << ") ";
	flp << ">= ";
	flp << 0;
	flp << endl;
      }
      // registers
      if(freg) {
	if(nregs > 0) {
	  // cardinality
	  flp << "c" << nclauses++ << ": ";
	  for(int k = 0; k < ndata; k++) {
	    flp << "+ ";
	    flp << "d(" << i << "," << j+npes << "," << k << ") ";
	  }
	  flp << "<= ";
	  flp << nregs;
	  flp << endl;
	}
	// communication possibility
	for(int k = 0; k < ndata; k++) {
	  flp << "c" << nclauses++ << ": ";
	  flp << "v(" << vVars[k] << ") ";
	  flp << "- ";
	  flp << "d(" << i << "," << j+npes << "," << k << ") ";
	  flp << ">= ";
	  flp << 0;
	  flp << endl;
	}
      }
    }
  }

  // conditions for output ready
  for(int k : output_ids) {
    flp << "c" << nclauses++ << ": ";
    flp << "d(" << ncycles-1 << "," << 0 << "," << k << ") ";
    flp << "= ";
    flp << 1;
    flp << endl;
  }

  flp << "binary" << endl;
  for(int i = 0; i < ncycles; i++) {
    for(int j = 0; j < nnodes_; j++) {
      for(int k = 0; k < ndata; k++) {
	flp << "d(" << i << "," << j << "," << k << ") ";
	flp << endl;
      }
    }
  }

  for(int i = 0; i < nvars; i++) {
    flp << "v(" << i << ") ";
    flp << endl;
  }
  
  flp << "end" << endl;
  flp.close();
}

void Gen::reduce_image() {
  vector<vector<vector<int> > > fimage(ncycles_, vector<vector<int> >(nnodes));
  for(int i = 0; i < ncycles_; i++) {
    for(int j = 0; j < nnodes; j++) {
      fimage[i][j].resize(image[i][j].size());
    }
  }
  // mark output data
  for(int i : output_ids) {
    int f = 0;
    for(int k = 0; k < image[ncycles_-1][0].size(); k++) {
      if(i == image[ncycles_-1][0][k]) {
	fimage[ncycles_-1][0][k] = 1;
	f = 1;
	break;
      }
    }
    if(!f) {
      show_error("solution is incomplete");
    }
  }
  // propagate backwards
  for(int i = ncycles_-1; i > 0; i--) {
    // back propagate from exmem
    for(int k = 0; k < image[i][0].size(); k++) {
      if(!fimage[i][0][k]) {
	continue;
      }
      int idk = image[i][0][k];
      int f = 0;
      // mark data in exmem at previous cycle if possible
      for(int l = 0; l < image[i-1][0].size(); l++) {
	int idl = image[i-1][0][l];
	if(idk == idl) {
	  fimage[i-1][0][l] = 1;
	  f = 1;
	  break;
	}
      }
      if(f) {
	continue;
      }
      // mark data coming from o-nodes
      for(int j : o_nodes) {
	for(int l = 0; l < image[i][j].size(); l++) {
	  int idl = image[i][j][l];
	  if(idk == idl) {
	    fimage[i][j][l] = 1;
	    f = 1;
	    break;
	  }
	}
	if(f) {
	  break;
	}
      }
      if(!f) {
	show_error("solution is incomplete");
      }
    }
    // back propagate from o-nodes to pe-nodes
    for(int j : o_nodes) {
      for(int k = 0; k < image[i][j].size(); k++) {
	if(!fimage[i][j][k]) {
	  continue;
	}
	int idk = image[i][j][k];
	int f = 0;
	for(int c : cons[j]) {
	  for(int l = 0; l < image[i-1][c].size(); l++) {
	    int idl = image[i-1][c][l];
	    if(idk == idl) {
	      fimage[i-1][c][l] = 1;
	      f = 1;
	      break;
	    }
	  }
	  if(f) {
	    break;
	  }
	}
	if(!f) {
	  show_error("solution is incomplete");
	}
      }
    }
    // back propagate from pe-nodes
    for(int j : pe_nodes) {
      for(int k = 0; k < image[i][j].size(); k++) {
	if(!fimage[i][j][k]) {
	  continue;
	}
	int idk = image[i][j][k];
	int f = 0;
	// mark data in the pe at previous cycle if possible
	for(int l = 0; l < image[i-1][j].size(); l++) {
	  int idl = image[i-1][j][l];
	  if(idk == idl) {
	    fimage[i-1][j][l] = 1;
	    f = 1;
	    break;
	  }
	}
	if(f) {
	  continue;
	}
	// mark data coming from adjacent nodes if possible
	for(int c : cons[j]) {
	  for(int l = 0; l < image[i-1][c].size(); l++) {
	    int idl = image[i-1][c][l];
	    if(idk == idl) {
	      fimage[i-1][c][l] = 1;
	      f = 1;
	      break;
	    }
	  }
	  if(f) {
	    break;
	  }
	}
	if(f) {
	  continue;
	}
	// mark data required for the operation
	for(auto s : operands[idk]) {
	  int g = 1;
	  // check if all operands are ready
	  for(auto ido : s) {
	    int h = 0;
	    // in the node
	    for(int l = 0; l < image[i-1][j].size(); l++) {
	      int idl = image[i-1][j][l];
	      if(ido == idl) {
		h = 1;
		break;
	      }
	    }
	    if(h) {
	      continue;
	    }
	    // in the adjacent nodes
	    for(int c : cons[j]) {
	      for(int l = 0; l < image[i-1][c].size(); l++) {
		int idl = image[i-1][c][l];
		if(ido == idl) {
		  h = 1;
		  break;
		}
	      }
	      if(h) {
		break;
	      }
	    }
	    if(!h) {
	      g = 0;
	      break;
	    }
	  }
	  if(!g) {
	    continue;
	  }
	  // mark operands
	  for(auto ido : s) {
	    int h = 0;
	    // in the node
	    for(int l = 0; l < image[i-1][j].size(); l++) {
	      int idl = image[i-1][j][l];
	      if(ido == idl) {
		fimage[i-1][j][l] = 1;
		h = 1;
		break;
	      }
	    }
	    if(h) {
	      continue;
	    }
	    // in the adjacent nodes
	    for(int c : cons[j]) {
	      for(int l = 0; l < image[i-1][c].size(); l++) {
		int idl = image[i-1][c][l];
		if(ido == idl) {
		  fimage[i-1][c][l] = 1;
		  h = 1;
		  break;
		}
	      }
	      if(h) {
		break;
	      }
	    }
	  }
	  f = 1;
	  break;
	}
	if(!f) {
	  show_error("solution is incomplete");
	}
      }
    }
    if(fexmem_) {
      // back propagate from i-nodes to exmem
      for(int j : i_nodes) {
	for(int k = 0; k < image[i][j].size(); k++) {
	  if(!fimage[i][j][k]) {
	    continue;
	  }
	  int idk = image[i][j][k];
	  int f = 0;
	  for(int l = 0; l < image[i-1][0].size(); l++) {
	    int idl = image[i-1][0][l];
	    if(idk == idl) {
	      fimage[i-1][0][l] = 1;
	      f = 1;
	      break;
	    }
	  }
	  if(!f) {
	    show_error("solution is incomplete");
	  }
	}
      }
    }
  }
  // update image
  vector<vector<vector<int> > > image_old = image;
  for(int i = 0; i < ncycles_; i++) {
    for(int j = 0; j < nnodes; j++) {
      image[i][j].clear();
      for(int k = 0; k < image_old[i][j].size(); k++) {
	if(fimage[i][j][k]) {
	  image[i][j].push_back(image_old[i][j][k]);
	}
      }
    }
  }
}
