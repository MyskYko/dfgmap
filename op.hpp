#ifndef OP_HPP
#define OP_HPP

#include <string>
#include <vector>
#include <map>
#include <set>

#include "global.hpp"

typedef struct opnode_ {
  int type;
  std::vector<struct opnode_ *> vc;
  int id;
} opnode;

class Dfg {
public:
  int ninputs = 0;
  std::vector<int> vcompressible;
  std::vector<std::string> outputnames;
  int ndata;
  std::vector<int> optypes;
  std::vector<std::set<std::set<int> > > operands;
  std::map<std::pair<int, std::multiset<int> >, int> unique;
  std::vector<std::string> datanames;

  int add_operator(std::string op, int n);
  void create_input(std::string name);
  opnode *create_opnode(std::string name, std::vector<std::string> &vs, int &pos);
  void gen_operands();
  void support_MAC();
  void print();
  int input_id(std::string name);
  void output_ids(std::set<int> &ids);
  void compress();
  
private:
  std::vector<std::string> operators;
  std::vector<int> noperands;
  std::map<std::string, opnode *> data_name2opnode;
  
  int optype(std::string s);
  std::string typeop(int i);
  int fcompressible(int i);
  void print_opnode(opnode * p, int depth);
  void compress_opnode(opnode * p);
  void gen_operands_opnode(opnode * p);
};

#endif // OP_HPP

