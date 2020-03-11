#include <algorithm>
#include <fstream>
#include <sstream>
#include <map>

#include "cnf.hpp"
#include "global.hpp"

using namespace std;

Cnf::Cnf(vector<int> pe_nodes, vector<int> mem_nodes, vector<tuple<vector<int>, vector<int>, int> > coms, int ninputs, set<int> output_ids, map<int, set<int> > assignments, vector<set<set<int> > > operands) :pe_nodes(pe_nodes), mem_nodes(mem_nodes), coms(coms), ninputs(ninputs), output_ids(output_ids), assignments(assignments), operands(operands)
{
  nnodes = pe_nodes.size() + mem_nodes.size();
  npes = pe_nodes.size();
  ndata = operands.size();
  ncoms = coms.size();
  incoms.resize(nnodes);
  outcoms.resize(nnodes);
  for(int i = 0; i < ncoms; i++) {
    for(int sender : get<0>(coms[i])) {
      outcoms[sender].insert(i);
    }
    for(int recipient : get<1>(coms[i])) {
      incoms[recipient].insert(i);
    }
  }
  for(int i : pe_nodes) {
    cout << i << " <- ";
    for(int com : incoms[i]) {
      cout << com << ",";
    }
    cout << " -> ";
    for(int com : outcoms[i]) {
      cout << com << ",";
    }
    cout << endl;
  }
}

void write_clause(vector<int> &vLits, ofstream &f) {
  for(int lit : vLits) {
    f << lit << " ";
  }
  f << 0 << endl;
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

void Cnf::gen_cnf(int ncycles, int nregs, int fexmem, int npipeline, string cnfname) {
  ncycles_ = ncycles;
  if(!npipeline) {
    npipeline = ncycles;
  }
  
  ofstream fcnf(cnfname);
  int nvars = ncycles * nnodes * ndata;
  yhead = nvars;
  nvars += ncycles * ncoms * ndata;
  zhead = nvars;
  nvars += ncycles * nnodes * ndata; // * nproc if not single core
  int nclauses = 0;
  
  vector<int> vLits;

  // init condition
  fcnf << "c init condition" << endl;
  for(int j = 0; j < nnodes; j++) {
    for(int i = 0; i < ndata; i++) {
      vLits.clear();
      if(assignments.count(j) && assignments[j].count(i)) {
	vLits.push_back(j*ndata + i + 1);
      }
      else {
	vLits.push_back(-(j*ndata + i + 1));
      }
      write_clause(vLits, fcnf);
      nclauses++;
    }
  }

  // final condition
  fcnf << "c final condition" << endl;
  for(int i : output_ids) {
    vLits.clear();
    vLits.push_back((ncycles-1)*nnodes*ndata + i + 1);
    write_clause(vLits, fcnf);
    nclauses++;
  }

  // conditions for PE
  fcnf << "c conditions for PE" << endl;
  for(int k = 1; k < ncycles; k++) {
    for(int j = 0; j < nnodes; j++) {
      for(int i = 0; i < ndata; i++) {
	vLits.clear();
	vLits.push_back(-(k*nnodes*ndata + j*ndata + i + 1));
	vLits.push_back((k-1)*nnodes*ndata + j*ndata + i + 1);
	for(int h : incoms[j]) {
	  vLits.push_back(yhead + k*ncoms*ndata + h*ndata + i + 1);
	}
	vLits.push_back(zhead + k*nnodes*ndata + j*ndata + i + 1);
	write_clause(vLits, fcnf);
	nclauses++;
      }
    }
  }

  // conditions for communication
  fcnf << "c conditions for communication" << endl;
  for(int k = 1; k < ncycles; k++) {
    for(int h = 0; h < ncoms; h++) {
      for(int i = 0; i < ndata; i++) {
	vLits.clear();
	vLits.push_back(-(yhead + k*ncoms*ndata + h*ndata + i + 1));
	for(int j : get<0>(coms[h])) {
	  vLits.push_back((k-1)*nnodes*ndata + j*ndata + i + 1);
	}
	write_clause(vLits, fcnf);
	nclauses++;
      }
    }
  }

  // conditions for operation
  fcnf << "c conditions for operation" << endl;
  for(int k = 1; k < ncycles; k++) {
    for(int j : pe_nodes) {
      for(int i = 0; i < ndata; i++) {
	for(auto s : operands[i]) {
	  // TODO : generate a new variable for each set
	  if(operands[i].size() != 1) { show_error("not 1 ope"); }
	  for(int d : s) {
	    vLits.clear();
	    vLits.push_back(-(zhead + k*nnodes*ndata + j*ndata + i + 1));
	    vLits.push_back((k-1)*nnodes*ndata + j*ndata + d + 1);
	    for(int h : incoms[j]) {
	      vLits.push_back(yhead + k*ncoms*ndata + h*ndata + d + 1);
	    }
	    write_clause(vLits, fcnf);
	    nclauses++;
	  }
	}
	// TODO : the variable -> OR of the new variables
	if(operands[i].empty()) {
	  vLits.clear();
	  vLits.push_back(-(zhead + k*nnodes*ndata + j*ndata + i + 1));
	  write_clause(vLits, fcnf);
	  nclauses++;
	}
      }
    }
    for(int j : mem_nodes) {
      for(int i = 0; i < ndata; i++) {
	vLits.clear();
	vLits.push_back(-(zhead + k*nnodes*ndata + j*ndata + i + 1));
	write_clause(vLits, fcnf);
	nclauses++;
      }
    }
  }

  // at most 1 or K
  fcnf << "c at most 1 or K" << endl;
  for(int t = 0; t < npipeline; t++) {
    // reg
    for(int j : pe_nodes) {
      vLits.clear();
      for(int k = t+1; k < ncycles; k += npipeline) {
	for(int i = 0; i < ndata; i++) {
	  vLits.push_back(k*nnodes*ndata + j*ndata + i + 1);
	}
      }
      if(nregs == 1) {
	cardinality_am1(nvars, nclauses, vLits, fcnf);
      }
      else {
	cardinality_amk(nvars, nclauses, vLits, fcnf, nregs);
      }
    }
    // com
    for(int h = 0; h < ncoms; h++) {
      vLits.clear();
      for(int k = t+1; k < ncycles; k += npipeline) {
	for(int i = 0; i < ndata; i++) {
	  vLits.push_back(yhead + k*ncoms*ndata + h*ndata + i + 1);
	}
      }
      int band = get<2>(coms[h]);
      if(band == 1) {
	cardinality_am1(nvars, nclauses, vLits, fcnf);	
      }
      else if(band > 0) {
	cardinality_amk(nvars, nclauses, vLits, fcnf, band);
      }
    }
    // ope
    for(int j : pe_nodes) {
      vLits.clear();
      for(int k = t+1; k < ncycles; k += npipeline) {
	for(int i = 0; i < ndata; i++) {
	  vLits.push_back(zhead + k*nnodes*ndata + j*ndata + i + 1);
	}
      }
      cardinality_am1(nvars, nclauses, vLits, fcnf); // branch if nproc used
    }
  }

  fcnf.close();

  string header = "p cnf " + to_string(nvars) + " " + to_string(nclauses);
  string cmd = "sed -i \'1i" + header + "\' " + cnfname;
  system(cmd.c_str());
}

/*
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
*/

/*
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
*/
