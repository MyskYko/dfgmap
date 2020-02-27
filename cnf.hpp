#ifndef CNF_HPP
#define CNF_HPP

#include <string>
#include <vector>
#include <set>

class Cnf {
public:
  std::vector<std::vector<std::vector<int> > > image;

  Cnf(std::vector<int> i_nodes, std::vector<int> o_nodes, std::vector<int> pe_nodes, std::vector<int> rom_nodes, std::set<std::pair<int, int> > &coms_, std::map<std::pair<int, int>, int> &com2band, int ninputs, std::set<int> output_ids, std::map<int, std::set<int> > assignments, std::vector<std::set<std::set<int> > > operands);
  
  void gen_cnf(int ncycles, int nregs, int fexmem, int npipeline, std::string cnfname);
  
  void gen_image(std::string rfilename);

  void reduce_image();

  std::map<int, std::set<int> > fixout;
  int finitread;
  
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
  int nc;
};

#endif // CNF_HPP
