#ifndef GEN_HPP
#define GEN_HPP

#include <string>
#include <vector>
#include <set>

class Gen {
public:
  std::vector<std::vector<std::vector<int> > > image;

  Gen(std::vector<int> i_nodes, std::vector<int> o_nodes, std::vector<int> pe_nodes, std::vector<std::set<int> > cons, int ninputs, std::set<int> output_ids, std::vector<std::set<std::set<int> > > operands);
  
  void gen_cnf(int ncycles, int nregs, int fexmem, std::string cnfname);
  void gen_cnf_exmem(int ncycles);
  void gen_cnf_reg_exmem(int ncycles, int nregs);
  
  void gen_ilp(int ncycles);
  
  void gen_image(std::string rfilename);

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

  int ncycles_;
  int nnodes_;
  int freg;
};

#endif // GEN_HPP
