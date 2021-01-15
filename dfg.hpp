#ifndef DFG_HPP
#define DFG_HPP

#include <string>
#include <vector>
#include <map>
#include <set>

class Dfg {
public:
  int get_ninputs() { return ninputs; }
  int get_ndata() { return ndata; }
  bool get_fmulti() { return fmulti; }
  std::string get_dataname(int i) { return datanames[i]; }
  std::vector<std::vector<std::set<int> > > get_operands() { return operands; }
  std::vector<std::tuple<int, int, bool> > get_priority();
  int get_nexs() { return nexs; }
  int get_nsels() { return nsels; }
  std::vector<std::tuple<bool, int, int, std::vector<int> > > get_exs() { return exs; }
  std::vector<std::vector<std::map<int, bool> > > get_exconds() { return exconds; }
  
  void read(std::string filename);
  void compress();
  void insert_xbtree();
  void gen_operands(bool fmultiopr, bool fname);

  void update_datanames(std::map<int, int> &exmap);
  
  int input_id(std::string name);
  std::set<int> output_ids();
  
  void print();
  void print_operands();

  ~Dfg();
  
private:
  typedef struct node_ {
    int type;
    std::vector<struct node_ *> vc;
    int id;
    bool dependent = false;
    std::vector<int> exind;
  } node;
  
  typedef struct opr_ {
    std::string s;
    int n;
    int attr;
  } opr;

  int ninputs = 0;
  int ndata;
  int nexs = 0;
  int nsels = 0;
  bool fmulti = 0;
  std::vector<std::string> datanames;
  std::vector<std::vector<std::set<int> > > operands;
  std::vector<node *> nodes;
  std::vector<std::string> outputnames;
  std::vector<opr *> oprs;
  std::map<std::string, node *> name2node;
  std::vector<node *> multioprs;
  std::vector<std::set<std::vector<int> > > operands_;
  std::vector<int> oprtypes;
  std::vector<bool> dependents;
  std::map<std::pair<int, std::vector<int> >, int> unique;
  std::vector<std::tuple<node *, node *, bool> > priority;
  std::vector<std::tuple<bool, int, int, std::vector<int> > > exs;
  std::vector<std::vector<std::map<int, bool> > > exconds;
  
  int oprtype(std::string s);
  std::string typeopr(int i);
  bool fcommutative(int i);
  bool fassociative(int i);
  node *create_multiopr(std::vector<std::string> &vs, int &pos);
  node *create_node(std::vector<std::string> &vs, int &pos);
  
  void create_opr(std::string s, int n, bool fc = 0, bool fa = 0);
  void create_input(std::string name);
  void create_multiopr(std::vector<std::string> &vs);
  void create_node(std::vector<std::string> &vs);
  
  void compress_node(node * p);
  void insert_xbtree_node(node * p);
  bool support_multiopr_rec(int id, node *ope, std::vector<std::set<int> > &vs, std::vector<std::map<int, bool> > &vm);
  void support_multiopr();
  void gen_operands_node(node * p, bool fname);
  
  void print_node(node * p, int depth, int exind);
};

#endif // DFG_HPP

