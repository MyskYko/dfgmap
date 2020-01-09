#ifndef OP_HPP
#define OP_HPP

#include <string>
#include <vector>
#include <map>
#include <set>

#include "global.hpp"

typedef struct opnode_ {
  int type; // input 0, + 1, * 2
  std::vector<struct opnode_ *> vc;
  int id;
} opnode;

int optype(std::string s);
std::string typeop(int i);
opnode *create_opnode(std::vector<std::string> &vs, int &pos, std::map<std::string, opnode *> &data_name2opnode);
void print_opnode(opnode * p, int depth);
void compress_opnode(opnode * p);
void gen_operands(opnode * p, int &ndata, std::vector<std::set<std::set<int> > > &operands, std::map<std::pair<int, std::multiset<int> >, int> &unique, std::vector<std::string> &datanames, int fmac);

std::string gen_opstr(opnode *p, std::vector<std::string> &datanames);
  
#endif // OP_HPP
