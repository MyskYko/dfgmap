#ifndef BLIF_HPP
#define BLIF_HPP

#include <string>
#include <vector>
#include <set>
#include <map>

class Blif {
public:
  int nsels;
  std::map<std::string, std::vector<std::pair<int, std::string> > > mcand;

  Blif(std::vector<std::string> inputnames, std::vector<std::string> outputnames);

  void gen_spec(std::string specfilename, std::vector<opnode *> &outputs);
  void gen_tmpl(std::string tmplfilename, int ncycles, int nregs, int nnodes, int nops, std::vector<opnode *> &operators, std::vector<std::pair<int, int> > &coms);
  void gen_top(std::string topfilename, std::string specfilename, std::string tmplfilename);
  void show_result(std::vector<int> &result);
  
private:
  std::vector<std::string> inputnames;
  std::vector<std::string> outputnames;

  int ncycles_;
  int nnodes_;
  int nregs_;
  std::vector<std::pair<int, int> > coms_;
};

#endif // BLIF_HPP
