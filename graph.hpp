#ifndef GRAPH_HPP
#define GRAPH_HPP

#include <set>
#include <map>
#include <string>
#include <vector>

class Graph {
public:
  int nnodes = 0;
  std::map<std::string, int> name2id;
  std::map<std::string, std::set<int> > nodes;
  std::map<std::string, std::vector<std::tuple<std::set<int>, std::set<int>, int> > > edges;

  void create_node(std::string type, std::string name);

  void read(std::string filename);
  
private:
  
};

#endif // GRAPH_HPP
