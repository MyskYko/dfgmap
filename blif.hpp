#ifndef BLIF_HPP
#define BLIF_HPP

#include <string>
#include <vector>
#include <set>
#include <map>

class Blif {
public:
  Blif(std::vector<std::string> inputnames, std::vector<std::string> outputnames, std::map<std::string, int> nodename2id);

  void gen_spec(std::string specfilename, std::vector<opnode *> &outputs);
  void gen_tmpl(std::string tmplfilename, int ncycles, int nregs, int nnodes, int nops, std::vector<opnode *> &operators, std::vector<std::pair<int, int> > &coms);
  void gen_top(std::string topfilename, std::string pfilename);
  int synthesize(std::string logfilename);
  void show_result();
  
private:
  std::vector<std::string> inputnames;
  std::vector<std::string> outputnames;
  std::map<std::string, int> nodename2id;
  
  std::string specfilename_;
  std::string tmplfilename_;
  std::string topfilename_;
  int ncycles_;
  int nnodes_;
  int nregs_;
  std::vector<std::pair<int, int> > coms_;
  
  int nsels;
  std::map<std::string, std::vector<std::pair<int, std::string> > > mcand;

  void write_constraints(std::ofstream &f, std::string pfilename);

  std::vector<int> result;
};

#endif // BLIF_HPP
