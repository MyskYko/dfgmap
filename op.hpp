#ifndef OP_HPP
#define OP_HPP

#include <string>
#include <vector>
#include <map>
#include <set>

#include "global.hpp"

class Dfg {
public:
  int ninputs = 0;
  std::vector<int> vcompressible;
  std::vector<std::string> outputnames;
  int ndata;
  std::vector<std::set<std::set<int> > > operands;
  std::vector<std::string> datanames;

  int add_operator(std::string op, int n);
  void create_input(std::string name);
  void create_opnode(std::vector<std::string> &vs);
  void print();
  int input_id(std::string name);
  void compress();
  void gen_operands();
  void support_MAC();
  std::set<int> output_ids();
  
private:
  typedef struct opnode_ {
    int type;
    std::vector<struct opnode_ *> vc;
    int id;
  } opnode;

  std::vector<opnode *> opnodes;
  std::vector<std::string> operators;
  std::vector<int> noperands;
  std::map<std::string, opnode *> data_name2opnode;
  std::vector<int> optypes;
  std::map<std::pair<int, std::multiset<int> >, int> unique;
  
  int optype(std::string s);
  std::string typeop(int i);
  int fcompressible(int i);
  opnode *create_opnode(std::vector<std::string> &vs, int &pos);
  void print_opnode(opnode * p, int depth);
  void compress_opnode(opnode * p);
  void gen_operands_opnode(opnode * p);
};

#endif // OP_HPP

