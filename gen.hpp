#ifndef GEN_HPP
#define GEN_HPP

#include <string>
#include <vector>
#include <set>

class Gen {
public:
  std::vector<std::vector<std::vector<int> > > image;

  Gen(std::vector<int> i_nodes, std::vector<int> o_nodes, std::vector<int> pe_nodes, std::vector<int> rom_nodes, std::set<std::pair<int, int> > &coms_, std::map<std::pair<int, int>, int> &com2band, int ninputs, std::set<int> output_ids, std::map<int, std::set<int> > assignments, std::vector<std::set<std::set<int> > > operands);
  
  void gen_cnf(int ncycles, int nregs, int fexmem, std::string cnfname);
  //  void gen_ilp(int ncycles, int nregs, int fexmem, std::string lpname);
  
  void gen_image(std::string rfilename);
  //  void gen_image_ilp(std::string sfilename);

  void reduce_image();

  std::map<int, std::set<int> > fixout;

private:
  int nnodes;
  int ndata;
  int ninputs;
  int npes;
  int ncoms;
  std::vector<int> i_nodes;
  std::vector<int> o_nodes;
  std::vector<int> pe_nodes;
  std::vector<int> rom_nodes;
  std::vector<std::tuple<int, int, int> > coms;
  std::set<int> output_ids;
  std::map<int, std::set<int> > assignments;
  std::vector<std::set<std::set<int> > > operands;

  std::vector<std::set<int> > cons;
  std::vector<std::set<int> > concoms;
  
  int ncycles_;
  int freg;
  std::map<int, int> pe2reg;
  int nr;
  int nc;
};

#endif // GEN_HPP
