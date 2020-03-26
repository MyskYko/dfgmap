#ifndef CNF_HPP
#define CNF_HPP

#include <string>
#include <vector>
#include <set>
#include <tuple>
#include <map>

class Cnf {
public:
  int nencode = 0;
  bool fmulti = 0;
  bool filp = 0;
  std::map<int, std::vector<bool> > assignments;
  std::vector<std::vector<std::vector<int> > > image;

  Cnf(std::set<int> pes, std::set<int> mem_nodes, std::vector<std::tuple<std::set<int>, std::set<int>, int> > coms, int ninputs, std::set<int> output_ids, std::vector<std::set<std::set<int> > > operands);

  void gen_cnf(int ncycles, int nregs, int nprocs, int fextmem, int ncontexts, std::string cnfname);

  void gen_image(std::string filename);

  void reduce_image();

private:
  int nnodes;
  int ndata;
  int ninputs;
  int ncoms;

  std::set<int> pes;
  std::set<int> mems;
  std::vector<std::tuple<std::set<int>, std::set<int>, int> > coms;
  std::set<int> output_ids;
  std::vector<std::set<std::set<int> > > operands;

  std::vector<std::set<int> > outcoms;
  std::vector<std::set<int> > incoms;
  
  int ncycles_;
  int nvars_;
  int yhead;
  int zhead;

  void write_clause(int &nclauses, std::vector<int> &vLits, std::ofstream &f);
  void amo_naive(int &nclauses, std::vector<int> &vLits, std::ofstream &f);
  void amo_bimander(int &nvars, int &nclauses, std::vector<int> &vLits, std::ofstream &f, int nbim);
  void amo_commander(int &nvars, int &nclauses, std::vector<int> vLits, std::ofstream &f);
  void cardinality_amo(int &nvars, int &nclauses, std::vector<int> &vLits, std::ofstream &f);
  void cardinality_amk(int &nvars, int &nclauses, std::vector<int> &vLits, std::ofstream &f, int k);
};
#endif // CNF_HPP
