#ifndef CNF_HPP
#define CNF_HPP

#include <string>
#include <vector>
#include <set>
#include <tuple>

class Cnf {
public:
  std::vector<std::vector<std::vector<int> > > image;

  Cnf(std::vector<int> pe_nodes, std::vector<int> mem_nodes, std::vector<std::tuple<std::vector<int>, std::vector<int>, int> > coms, int ninputs, std::set<int> output_ids, std::map<int, std::set<int> > assignments, std::vector<std::set<std::set<int> > > operands);

  void gen_cnf(int ncycles, int nregs, int fextmem, int npipeline, std::string cnfname);

  void gen_image(std::string rfilename);

  //  void reduce_image();
  
private:
  int nnodes;
  int ndata;
  int ninputs;
  int npes;
  int ncoms;

  std::vector<int> pe_nodes;
  std::vector<int> mem_nodes;
  std::vector<std::tuple<std::vector<int>, std::vector<int>, int> > coms;
  std::set<int> output_ids;
  std::map<int, std::set<int> > assignments;
  std::vector<std::set<std::set<int> > > operands;

  std::vector<std::set<int> > outcoms;
  std::vector<std::set<int> > incoms;
  
  int ncycles_;
  int yhead;
  int zhead;
};

#endif // CNF_HPP
