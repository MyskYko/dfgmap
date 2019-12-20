#ifndef SAT_HPP
#define SAT_HPP

#include <string>
#include <vector>
#include <set>

#include "utils/System.h"
#include "utils/ParseUtils.h"
#include "utils/Options.h"
#include "core/Dimacs.h"
#include "simp/SimpSolver.h"

class Sat {
public:
  std::vector<std::vector<std::vector<int> > > image;

  Sat(std::vector<int> i_nodes, std::vector<int> o_nodes, std::vector<int> pe_nodes, std::vector<std::set<int> > cons, int ninputs, std::set<int> output_ids, std::vector<std::set<std::set<int> > > operands);
  
  void gen_cnf(int ncycles);
  void gen_cnf_exmem(int ncycles);
  void gen_cnf_reg(int ncycles, int nregs);
  void gen_cnf_reg_exmem(int ncycles, int nregs);
  
  void gen_ilp(int ncycles);
  
  bool solve() { return S->solve(); };
  void gen_image();

  void write(std::string cfilename) { S->toDimacs(cfilename.c_str()); };
  void clean() { delete S; S = new Glucose::SimpSolver; };
private:
  int nnodes;
  int ndata;
  int ninputs;
  std::vector<int> i_nodes;
  std::vector<int> o_nodes;
  std::vector<int> pe_nodes;
  std::vector<std::set<int> > cons;
  std::set<int> output_ids;
  std::vector<std::set<std::set<int> > > operands;

  Glucose::SimpSolver *S;
  
  int ncycles_;
  int freg;
};

#endif // SAT_HPP
