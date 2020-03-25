#ifndef GRAPH_HPP
#define GRAPH_HPP

#include <set>
#include <map>
#include <tuple>
#include <string>
#include <vector>
#include <algorithm>

class Graph {
public:
  int get_nnodes() { return nnodes; }
  std::set<int> get_nodes(std::string type) { return nodes[type]; }
  std::vector<std::tuple<std::set<int>, std::set<int>, int> > get_edges(std::string type) { return edges[type]; }
  
  int get_id(std::string name) {
    if(!name2id.count(name)) {
      return -1;
    }
    return name2id[name];
  }
  
  std::map<int, std::string> get_id2name() {
    std::map<int, std::string> m;
    for(auto i : name2id) {
      m[i.second] = i.first;
    }
    return m;
  }

  std::string get_type(int id) {
    for(auto &i : nodes) {
      if(std::find(i.second.begin(), i.second.end(), id) != i.second.end()) {
	return i.first;
      }
    }
    return "";
  }

  std::vector<std::string> get_types() {
    std::vector<std::string> v;
    for(auto &i : nodes) {
      v.push_back(i.first);
    }
    return v;
  }
  
  void create_node(std::string type, std::string name);
  void read(std::string filename);
  void print();
  
private:
  int nnodes = 0;
  std::map<std::string, int> name2id;
  std::map<std::string, std::set<int> > nodes;
  std::map<std::string, std::vector<std::tuple<std::set<int>, std::set<int>, int> > > edges;
};

#endif // GRAPH_HPP
