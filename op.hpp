#ifndef OP_HPP
#define OP_HPP

#include <string>
#include <vector>
#include <map>
#include <set>

#include "global.hpp"

typedef struct opnode_ {
  int type; // input 0
  std::vector<struct opnode_ *> vc;
  int id;
} opnode;

class Op {
public:
  static std::vector<int> vcompressible;

  static int add_operator(std::string op, int n);
  static opnode *create_opnode(std::vector<std::string> &vs, int &pos, std::map<std::string, opnode *> &data_name2opnode);
  static void print_opnode(opnode * p, int depth);
  static void compress_opnode(opnode * p);
  static void gen_operands(opnode * p, int &ndata, std::vector<int> &optypes, std::vector<std::set<std::set<int> > > &operands, std::map<std::pair<int, std::multiset<int> >, int> &unique, std::vector<std::string> &datanames);
  static void support_MAC(std::vector<int> &optypes, std::vector<std::set<std::set<int> > > &operands);
  
private:
  static std::vector<std::string> operators;
  static std::vector<int> noperands;
  
  static int optype(std::string s);
  static std::string typeop(int i);
  static int fcompressible(int i);
};

#endif // OP_HPP
