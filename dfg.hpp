#ifndef DFG_HPP
#define DFG_HPP

#include <string>
#include <vector>
#include <map>
#include <set>

#include "global.hpp"

class Dfg {
public:
  int ninputs = 0;
  std::vector<std::string> outputnames;
  int ndata;
  int fmulti = 0;
  std::vector<std::set<std::set<int> > > operands;
  std::vector<std::string> datanames;

  int add_operator(std::string s, int n);
  void make_commutative(int i);
  void make_associative(int i);
  void create_input(std::string name);
  void add_multiope(std::vector<std::string> &vs);
  void create_opnode(std::vector<std::string> &vs);
  void print();
  int input_id(std::string name);
  void compress();
  void gen_operands();
  void support_multiope();
  std::set<int> output_ids();
  
private:
  typedef struct opnode_ {
    int type;
    std::vector<struct opnode_ *> vc;
    int id;
  } opnode;

  typedef struct opr_ {
    std::string s;
    int n;
    int attr;
  } opr;

  std::vector<opnode *> opnodes;
  std::vector<opr *> oprs;
  std::map<std::string, opnode *> data_name2opnode;
  std::vector<opnode *> multiopes;
  std::vector<std::set<std::vector<int> > > voperands;
  std::vector<int> optypes;
  std::map<std::pair<int, std::vector<int> >, int> unique;
  
  int optype(std::string s);
  std::string typeop(int i);
  int fcommutative(int i);
  int fassociative(int i);
  opnode *create_multiope(std::vector<std::string> &vs, int &pos);
  opnode *create_opnode(std::vector<std::string> &vs, int &pos);
  void print_opnode(opnode * p, int depth);
  void compress_opnode(opnode * p);
  void gen_operands_opnode(opnode * p);
  int support_multiope_rec(int id, opnode *ope, std::vector<std::set<int> > &vs);
};

#endif // DFG_HPP

