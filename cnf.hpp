#ifndef CNF_HPP
#define CNF_HPP

#include <string>
#include <vector>
#include <set>
#include <tuple>
#include <map>

#include <simp/SimpSolver.h>

class Cnf {
public:
  int nencode = 0;
  bool fmulti = 0;
  int nilp = 0;
  std::map<int, std::vector<int> > assignments;
  std::vector<std::tuple<int, std::set<int>, int> > amk_assignments;
  std::map<int, int> memsize;
  std::map<int, std::pair<int, int> > nports;
  std::vector<int> tempnodes;
  std::vector<std::tuple<int, int, bool> > priority;
  std::vector<std::vector<std::vector<int> > > image;
  std::map<int, int> exmap;

  int nexs = 0;
  int nsels = 0;
  std::vector<std::tuple<bool, int, int, std::vector<int> > > exs;
  std::vector<std::vector<std::map<int, bool> > > exconds;

  Cnf(std::set<int> pes, std::set<int> mem_nodes, std::vector<std::tuple<std::set<int>, std::set<int>, int> > coms, int ninputs, std::set<int> output_ids, std::vector<std::vector<std::set<int> > > operands);

  void support_port();
  
  void gen_cnf(int ncycles, int nregs, int nprocs, int fextmem, int ncontexts, std::string cnfname);

  void gen_image(std::string filename);

  void reduce_image();

  void setup_glucose(bool fincr);
  int run_glucose() { return S->solve(); }
  void run_glucose_opt();

  ~Cnf() {
    if(S) {
      delete S;
    }
  }

private:
  int nnodes;
  int ndata;
  int ninputs;
  int ncoms;

  std::set<int> pes;
  std::set<int> mems;
  std::vector<std::tuple<std::set<int>, std::set<int>, int> > coms;
  std::set<int> output_ids;
  std::vector<std::vector<std::set<int> > > operands;

  std::vector<std::set<int> > outcoms;
  std::vector<std::set<int> > incoms;

  std::map<int, std::set<int> > bypass;

  std::ofstream f;
  Glucose::SimpSolver *S = NULL;
  
  int ncycles_;
  int nvars_;
  int yhead;
  int zhead;

  int phead;
  int qhead;

  void write_clause(int &nclauses, std::vector<int> &vLits);
  void amo_naive(int &nclauses, std::vector<int> &vLits);
  void amo_bimander(int &nvars, int &nclauses, std::vector<int> &vLits, int nbim);
  void amo_commander(int &nvars, int &nclauses, std::vector<int> vLits);
  void cardinality_amo(int &nvars, int &nclauses, std::vector<int> &vLits);
  void cardinality_amk(int &nvars, int &nclauses, std::vector<int> &vLits, int k);
};
#endif // CNF_HPP
