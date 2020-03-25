#ifndef CNF_HPP
#define CNF_HPP

#include <string>
#include <vector>
#include <set>
#include <tuple>

class Cnf {
public:
  int fmultiop;
  int nencode;
  int filp;
  std::vector<std::vector<std::vector<int> > > image;

  Cnf(std::set<int> pes, std::set<int> mem_nodes, std::vector<std::tuple<std::set<int>, std::set<int>, int> > coms, int ninputs, std::set<int> output_ids, std::map<int, std::set<int> > assignments, std::vector<std::set<std::set<int> > > operands);

  void gen_cnf(int ncycles, int nregs, int nprocs, int fextmem, int npipeline, std::string cnfname);

  void gen_image(std::string rfilename);

  //  void reduce_image();

private:
  int nnodes;
  int ndata;
  int ninputs;
  int ncoms;

  std::set<int> pes;
  std::set<int> mems;
  std::vector<std::tuple<std::set<int>, std::set<int>, int> > coms;
  std::set<int> output_ids;
  std::map<int, std::set<int> > assignments;
  std::vector<std::set<std::set<int> > > operands;

  std::vector<std::set<int> > outcoms;
  std::vector<std::set<int> > incoms;
  
  int ncycles_;
  int yhead;
  int zhead;

  void write_clause(int &nclauses, std::vector<int> &vLits, std::ofstream &f);
  void amo_naive(int &nclauses, std::vector<int> &vLits, std::ofstream &fcnf);
  void amo_bimander(int &nvars, int &nclauses, std::vector<int> &vLits, std::ofstream &fcnf, int nbim);
  void amo_commander(int &nvars, int &nclauses, std::vector<int> vLits, std::ofstream &fcnf);
  void cardinality_amo(int &nvars, int &nclauses, std::vector<int> &vLits, std::ofstream &fcnf);
  void cardinality_amk(int &nvars, int &nclauses, std::vector<int> vLits, std::ofstream &fcnf, int k);
};
#endif // CNF_HPP
