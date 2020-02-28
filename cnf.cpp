#include <algorithm>
#include <fstream>
#include <sstream>
#include <map>

#include "cnf.hpp"
#include "global.hpp"

using namespace std;

Cnf::Cnf(vector<int> i_nodes, vector<int> o_nodes, vector<int> pe_nodes, vector<int> rom_nodes, set<pair<int, int> > &coms_, map<pair<int, int>, int> &com2band, int ninputs, set<int> output_ids, map<int, set<int> > assignments, vector<set<set<int> > > operands) : i_nodes(i_nodes), o_nodes(o_nodes), pe_nodes(pe_nodes), rom_nodes(rom_nodes), ninputs(ninputs), output_ids(output_ids), assignments(assignments), operands(operands)
{
  nnodes = 1 + i_nodes.size() + o_nodes.size() + pe_nodes.size() + rom_nodes.size();
  npes = pe_nodes.size();
  ndata = operands.size();
  ncoms = 0;
  cons.resize(nnodes);
  concoms.resize(nnodes);
  for(auto com : coms_) {
    cons[com.second].insert(com.first);
  }
  for(int i : i_nodes) {
    if(cons[i].size()) {
      show_error("there is an input node with input");
    }
  }
  for(int i : rom_nodes) {
    if(cons[i].size()) {
      show_error("there is a ROM node with input");
    }
  }
  for(int i : o_nodes) {
    if(cons[i].empty()) {
      show_error("there is an output node without input");
    }	     
    for(int con : cons[i]) {
      if(find(pe_nodes.begin(), pe_nodes.end(), con) == pe_nodes.end()) {
	show_error("there is a path from a non PE node to an output node");
      }
      if(com2band[make_pair(con, i)]) {
	cout << "bandwidth of a path to an output node is ignored" << endl;
      }
    }
  }
  for(int i : pe_nodes) {
    if(cons[i].empty()) {
      show_error("there is a PE node without input");
    }
    for(int con : cons[i]) {
      if(find(o_nodes.begin(), o_nodes.end(), con) != o_nodes.end()) {
	show_error("there is a path from an output node to a PE node");
      }
    }
    for(auto it = cons[i].begin(); it != cons[i].end();) {
      auto com = make_pair(*it, i);
      if(com2band[com]) {
	concoms[i].insert(ncoms++);
	coms.push_back(make_tuple(*it, i, com2band[com]));
	it = cons[i].erase(it);
      }
      else {
	it++;
      }
    }
  }
  /*
  for(int i : pe_nodes) {
    cout << i << " <- ";
    for(int con : cons[i]) {
      cout << con << ",";
    }
    for(int com : concoms[i]) {
      cout << "c" << com << ",";
    }
    cout << endl;
  }
  for(auto com : coms) {
    cout << get<0>(com) << " -> " << get<1>(com) << " : " << get<2>(com) << endl;
  }
  */
}

void cardinality_am1(int &nvars, int &nclauses, vector<int> vVars, ofstream &fcnf) {
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
}

void cardinality_amk(int &nvars, int &nclauses, vector<int> vVars, ofstream &fcnf, int k) {
  if(vVars.empty()) {
    return;
  }
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
}

void Cnf::gen_image(string rfilename) {
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
    }
    else if(s == "v") {
      while(getline(ss, s, ' ')) {
	try {
	  results.push_back(stoi(s));
	} catch(...) {
	  show_error("wrong formatted result file");
	}
      }
    }
  }
  
  image.resize(ncycles_, vector<vector<int> >(nnodes + ncoms));
  for(int k = 0; k < ncycles_; k++) {
    for(int j = 0; j < nnodes; j++) {
      for(int i = 0; i < ndata; i++) {
	if(results[k*nnodes*ndata + j*ndata + i] > 0) {
	  image[k][j].push_back(i);
	}
      }
    }
  }
  for(int k = 0; k < ncycles_; k++) {
    for(int h = 0; h < ncoms; h++) {
      for(int i = 0; i < ndata; i++) {
	if(results[nc + k*ncoms*ndata + h*ndata + i] > 0) {
	  image[k][nnodes+h].push_back(i);
	}
      }
    }
  }
}

void Cnf::gen_cnf(int ncycles, int nregs, int fexmem, int npipeline, string cnfname) {
  ncycles_ = ncycles;
  if(!npipeline) {
    npipeline = ncycles;
  }
  
  ofstream fcnf(cnfname);
  int nvars = ncycles * nnodes * ndata;
  nc = nvars;
  nvars += ncycles * ncoms * ndata;
  int nclauses = 0;

  // init condition
  fcnf << "c init condition" << endl;
  for(int j = 0; j < nnodes; j++) {
    for(int i = 0; i < ndata; i++) {
      if(assignments.count(j) && assignments[j].count(i)) {
	fcnf << j*ndata + i + 1 << " ";
	fcnf << 0 << endl; 
	nclauses++;
      }
      else {
	fcnf << "-" << j*ndata + i + 1 << " ";
	fcnf << 0 << endl; 
	nclauses++;
      }
    }
  }
  
  // conditions for input nodes
  fcnf << "c conditions for input nodes" << endl;
  for(int k = 1; k < ncycles; k++) {
    for(int j : i_nodes) {
      for(int i = 0; i < ninputs; i++) {
	fcnf << "-" << k*nnodes*ndata + j*ndata + i + 1 << " ";
	fcnf << (k-1)*nnodes*ndata + i + 1 << " ";
	fcnf << 0 << endl; 
	nclauses++;
      }
      for(int i = ninputs; i < ndata; i++) {
	fcnf << "-" << k*nnodes*ndata + j*ndata + i + 1 << " ";
	if(fexmem) {
	  fcnf << (k-1)*nnodes*ndata + i + 1 << " ";
	}
	fcnf << 0 << endl; 
	nclauses++;
      }
    }
  }

  // conditions for external memory
  fcnf << "c conditions for external memory" << endl;
  for(int k = 1; k < ncycles; k++) {
    for(int i = 0; i < ndata; i++) {
      fcnf << "-" << k*nnodes*ndata + i + 1 << " ";
      fcnf << (k-1)*nnodes*ndata + i + 1 << " ";
      for(int j : o_nodes) {
	fcnf << k*nnodes*ndata + j*ndata + i + 1 << " ";
      }
      fcnf << 0 << endl; 
      nclauses++;
    }
  }

  // conditions for operation and PE nodes
  fcnf << "c conditions for operation and PE nodes" << endl;
  vector<vector<vector<int> > > vz;
  vz.resize(ncycles);
  for(int k = 1; k < ncycles; k++) {
    vz[k].resize(nnodes);
    for(int j : pe_nodes) {
      for(int i = 0; i < ndata; i++) {
	// conditions for operation
	vector<int> vVars;
	for(auto d : operands[i]) {
	  nvars++;
	  for(int a : d) {
	    fcnf << "-" << nvars << " ";
	    fcnf << (k-1)*nnodes*ndata + j*ndata + a + 1 << " ";
	    for(int con : cons[j]) {
	      fcnf << (k-1)*nnodes*ndata + con*ndata + a + 1 << " ";
	    }
	    for(int com : concoms[j]) {
	      fcnf << nc + (k-1)*ncoms*ndata + com*ndata + a + 1 << " ";
	    }
	    fcnf << 0 << endl;
	    nclauses++;
	  }
	  vVars.push_back(nvars);
	}
	// conditions for PE nodes
	fcnf << "-" << k*nnodes*ndata + j*ndata + i + 1 << " ";
	fcnf << (k-1)*nnodes*ndata + j*ndata + i + 1 << " ";
	for(int con : cons[j]) {
	  fcnf << (k-1)*nnodes*ndata + con*ndata + i + 1 << " ";
	}
	for(int com : concoms[j]) {
	  fcnf << nc + (k-1)*ncoms*ndata + com*ndata + i + 1 << " ";
	}
	for(int z : vVars) {
	  fcnf << z << " ";
	  vz[k][j].push_back(z);
	}
	fcnf << 0 << endl;
	nclauses++;
      }
    }
  }

  // conditions for output nodes
  fcnf << "c conditions for output nodes" << endl;
  for(int k = 1; k < ncycles; k++) {
    for(int j : o_nodes) {
      for(int i = 0; i < ndata; i++) {
	fcnf << "-" << k*nnodes*ndata + j*ndata + i + 1 << " ";
	//	fcnf << (k-1)*nnodes*ndata + j*ndata + i + 1 << " ";
	for(int con : cons[j]) {
	  fcnf << (k-1)*nnodes*ndata + con*ndata + i + 1 << " ";
	}
	fcnf << 0 << endl;
	nclauses++;
      }
    }
  }

  // conditions for ROM nodes
  fcnf << "c conditions for ROM nodes" << endl;
  for(int j : rom_nodes) {
    for(int i = 0; i < ndata; i++) {
      if(assignments.count(j) && assignments[j].count(i)) {
	for(int k = 1; k < ncycles; k++) {
	  fcnf << k*nnodes*ndata + j*ndata + i + 1 << " ";
	  fcnf << 0 << endl;
	  nclauses++;
	}
      }
      else {
	for(int k = 1; k < ncycles; k++) {
	  fcnf << "-" << k*nnodes*ndata + j*ndata + i + 1 << " ";
	  fcnf << 0 << endl; 
	  nclauses++;
	}
      }
    }
  }

  // conditions for communication
  fcnf << "c conditions for communication" << endl;
  for(int k = 0; k < ncycles; k++) {
    for(int h = 0; h < ncoms; h++) {
      for(int i = 0; i < ndata; i++) {
	fcnf << "-" << nc + k*ncoms*ndata + h*ndata + i + 1 << " ";
	int j = get<0>(coms[h]);
	fcnf << k*nnodes*ndata + j*ndata + i + 1 << " ";
	fcnf << 0 << endl;
	nclauses++;
      }
    }
  }

  // at most 1 or K
  fcnf << "c at most 1 or K" << endl;
  for(int t = 0; t < npipeline; t++) {
    for(int j : i_nodes) {
      vector<int> vVars;
      for(int k = t+1; k < ncycles; k += npipeline) {
	for(int i = 0; i < ndata; i++) {
	  vVars.push_back(k*nnodes*ndata + j*ndata + i + 1);
	}
      }
      cardinality_am1(nvars, nclauses, vVars, fcnf);
    }
    for(int j : o_nodes) {
      vector<int> vVars;
      for(int k = t+1; k < ncycles; k += npipeline) {
	for(int i = 0; i < ndata; i++) {
	  vVars.push_back(k*nnodes*ndata + j*ndata + i + 1);
	}
      }
      cardinality_am1(nvars, nclauses, vVars, fcnf);
    }
    for(int j : pe_nodes) {
      vector<int> vVars;
      for(int k = t+1; k < ncycles; k += npipeline) {
	for(int i = 0; i < ndata; i++) {
	  vVars.push_back(k*nnodes*ndata + j*ndata + i + 1);
	}
      }
      if(nregs == 1) {
	cardinality_am1(nvars, nclauses, vVars, fcnf);
      }
      else {
	cardinality_amk(nvars, nclauses, vVars, fcnf, nregs);
      }
    }
    for(int h = 0; h < ncoms; h++) {
      vector<int> vVars;
      for(int k = t; k < ncycles; k += npipeline) {
	for(int i = 0; i < ndata; i++) {
	  vVars.push_back(nc + k*ncoms*ndata + h*ndata + i + 1);
	}
      }
      int band = get<2>(coms[h]);
      if(band == 1) {
	cardinality_am1(nvars, nclauses, vVars, fcnf);	
      }
      else {
	cardinality_amk(nvars, nclauses, vVars, fcnf, band);
      }
    }
    for(int j : pe_nodes) {
      vector<int> vVars;
      for(int k = t+1; k < ncycles; k += npipeline) {
	for(int z : vz[k][j]) {
	  vVars.push_back(z);
	}
      }
      cardinality_am1(nvars, nclauses, vVars, fcnf);
    }
  }

  // final condition
  fcnf << "c final condition" << endl;
  for(int i : output_ids) {
    fcnf << (ncycles-1)*nnodes*ndata + i + 1 << " ";
    fcnf << 0 << endl;
    nclauses++;
  }

  // fixout
  if(fixout.size()) {
    fcnf << "c fixout" << endl;
    for(int k = 0; k < ncycles; k++) {
      for(auto elem : fixout) {
	int j = elem.first;
	for(int i = 0; i < ndata; i++) {
	  if(elem.second.count(i)) {
	    continue;
	  }
	  fcnf << "-" << k*nnodes*ndata + j*ndata + i + 1 << " ";
	  fcnf << 0 << endl;
	  nclauses++;
	}
      }
    }
  }
  
  // initread
  if(finitread) {
    fcnf << "c initread" << endl;
    for(int i = 0; i < ndata; i++) {
      vector<int> vVars;
      for(int j : i_nodes) {
	vVars.push_back(nnodes*ndata + j*ndata + i + 1);
      }
      cardinality_am1(nvars, nclauses, vVars, fcnf);
    }
    for(int k = 2; k < ncycles; k++) {
      for(int j : i_nodes) {
	for(int i = 0; i < ndata; i++) {
	  fcnf << "-" << k*nnodes*ndata + j*ndata + i + 1 << " ";
	  fcnf << 0 << endl;
	  nclauses++;
	}
      }
    }
  }
  
  fcnf.close();

  string header = "p cnf " + to_string(nvars) + " " + to_string(nclauses);
  string cmd = "sed -i \'1i" + header + "\' " + cnfname;
  system(cmd.c_str());
}

void Cnf::reduce_image() {
  vector<vector<vector<int> > > fimage(ncycles_, vector<vector<int> >(nnodes + ncoms));
  for(int k = 0; k < ncycles_; k++) {
    for(int j = 0; j < nnodes + ncoms; j++) {
      fimage[k][j].resize(image[k][j].size());
    }
  }
  // mark output data
  for(int id : output_ids) {
    int f = 0;
    for(int i = 0; i < image[ncycles_-1][0].size(); i++) {
      if(id == image[ncycles_-1][0][i]) {
	fimage[ncycles_-1][0][i] = 1;
	f = 1;
	break;
      }
    }
    if(!f) {
      show_error("solution is incomplete");
    }
  }
  // propagate backwards
  for(int k = ncycles_-1; k > 0; k--) {
    // back propagate from exmem
    for(int i = 0; i < image[k][0].size(); i++) {
      if(!fimage[k][0][i]) {
	continue;
      }
      int idi = image[k][0][i];
      int f = 0;
      // mark data in exmem at previous cycle if possible
      for(int ii = 0; ii < image[k-1][0].size(); ii++) {
	int idii = image[k-1][0][ii];
	if(idi == idii) {
	  fimage[k-1][0][ii] = 1;
	  f = 1;
	  break;
	}
      }
      if(f) {
	continue;
      }
      // mark data coming from o-nodes
      for(int j : o_nodes) {
	for(int ii = 0; ii < image[k][j].size(); ii++) {
	  int idii = image[k][j][ii];
	  if(idi == idii) {
	    fimage[k][j][ii] = 1;
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
      for(int i = 0; i < image[k][j].size(); i++) {
	if(!fimage[k][j][i]) {
	  continue;
	}
	int idi = image[k][j][i];
	int f = 0;
	for(int c : cons[j]) {
	  for(int ii = 0; ii < image[k-1][c].size(); ii++) {
	    int idii = image[k-1][c][ii];
	    if(idi == idii) {
	      fimage[k-1][c][ii] = 1;
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
    // back propagate from communication paths
    for(int h = 0; h < ncoms; h++) {
      for(int i = 0; i < image[k][nnodes+h].size(); i++) {
	if(!fimage[k][nnodes+h][i]) {
	  continue;
	}
	int idi = image[k][nnodes+h][i];
	int f = 0;
	int j = get<0>(coms[h]);
	for(int ii = 0; ii < image[k][j].size(); ii++) {
	  int idii = image[k][j][ii];
	  if(idi == idii) {
	    fimage[k][j][ii] = 1;
	    f = 1;
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
      for(int i = 0; i < image[k][j].size(); i++) {
	if(!fimage[k][j][i]) {
	  continue;
	}
	int idi = image[k][j][i];
	int f = 0;
	// mark data in the pe at previous cycle if possible
	for(int ii = 0; ii < image[k-1][j].size(); ii++) {
	  int idii = image[k-1][j][ii];
	  if(idi == idii) {
	    fimage[k-1][j][ii] = 1;
	    f = 1;
	    break;
	  }
	}
	if(f) {
	  continue;
	}
	// mark data coming from adjacent nodes if possible
	for(int c : cons[j]) {
	  for(int ii = 0; ii < image[k-1][c].size(); ii++) {
	    int idii = image[k-1][c][ii];
	    if(idi == idii) {
	      fimage[k-1][c][ii] = 1;
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
	// mark data coming from communication paths
	for(int h : concoms[j]) {
	  for(int ii = 0; ii < image[k-1][nnodes+h].size(); ii++) {
	    int idii = image[k-1][nnodes+h][ii];
	    if(idi == idii) {
	      fimage[k-1][nnodes+h][ii] = 1;
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
	for(auto d : operands[idi]) {
	  int ff = 1;
	  // check if all operands are ready
	  for(auto a : d) {
	    int fff = 0;
	    // in the node
	    for(int ii = 0; ii < image[k-1][j].size(); ii++) {
	      int idii = image[k-1][j][ii];
	      if(a == idii) {
		fff = 1;
		break;
	      }
	    }
	    if(fff) {
	      continue;
	    }
	    // in the adjacent nodes
	    for(int c : cons[j]) {
	      for(int ii = 0; ii < image[k-1][c].size(); ii++) {
		int idii = image[k-1][c][ii];
		if(a == idii) {
		  fff = 1;
		  break;
		}
	      }
	      if(fff) {
		break;
	      }
	    }
	    if(fff) {
	      continue;
	    }
	    // in the communication paths
	    for(int h : concoms[j]) {
	      for(int ii = 0; ii < image[k-1][nnodes+h].size(); ii++) {
		int idii = image[k-1][nnodes+h][ii];
		if(a == idii) {
		  fff = 1;
		  break;
		}
	      }
	      if(fff) {
		break;
	      }
	    }
	    if(fff) {
	      continue;
	    }
	    ff = 0;
	    break;
	  }
	  if(!ff) {
	    continue;
	  }
	  // mark operands
	  for(auto a : d) {
	    int fff = 0;
	    // in the node
	    for(int ii = 0; ii < image[k-1][j].size(); ii++) {
	      int idii = image[k-1][j][ii];
	      if(a == idii) {
		fimage[k-1][j][ii] = 1;
		fff = 1;
		break;
	      }
	    }
	    if(fff) {
	      continue;
	    }
	    // in the adjacent nodes
	    for(int c : cons[j]) {
	      for(int ii = 0; ii < image[k-1][c].size(); ii++) {
		int idii = image[k-1][c][ii];
		if(a == idii) {
		  fimage[k-1][c][ii] = 1;
		  fff = 1;
		  break;
		}
	      }
	      if(fff) {
		break;
	      }
	    }
	    if(fff) {
	      continue;
	    }
	    // in the communication paths
	    for(int h : concoms[j]) {
	      for(int ii = 0; ii < image[k-1][nnodes+h].size(); ii++) {
		int idii = image[k-1][nnodes+h][ii];
		if(a == idii) {
		  fimage[k-1][nnodes+h][ii] = 1;
		  fff = 1;
		  break;
		}
	      }
	      if(fff) {
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
    // back propagate from i-nodes to exmem
    for(int j : i_nodes) {
      for(int i = 0; i < image[k][j].size(); i++) {
	if(!fimage[k][j][i]) {
	  continue;
	}
	int idi = image[k][j][i];
	int f = 0;
	for(int ii = 0; ii < image[k-1][0].size(); ii++) {
	  int idii = image[k-1][0][ii];
	  if(idi == idii) {
	    fimage[k-1][0][ii] = 1;
	    f = 1;
	    break;
	  }
	}
	if(!f) {
	  show_error("solution is incomplete");
	}
      }
    }
    // back propagate in rom
    for(int j : rom_nodes) {
      for(int i = 0; i < image[k][j].size(); i++) {
	if(!fimage[k][j][i]) {
	  continue;
	}
	int idi = image[k][j][i];
	int f = 0;
	for(int ii = 0; ii < image[k-1][j].size(); ii++) {
	  int idii = image[k-1][j][ii];
	  if(idi == idii) {
	    fimage[k-1][j][ii] = 1;
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
  // update image
  vector<vector<vector<int> > > image_old = image;
  for(int k = 0; k < ncycles_; k++) {
    for(int j = 0; j < nnodes + ncoms; j++) {
      image[k][j].clear();
      for(int i = 0; i < image_old[k][j].size(); i++) {
	if(fimage[k][j][i]) {
	  image[k][j].push_back(image_old[k][j][i]);
	}
      }
    }
  }
}
