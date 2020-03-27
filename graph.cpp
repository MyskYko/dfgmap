#include <fstream>
#include <sstream>

#include "global.hpp"
#include "graph.hpp"

using namespace std;

void Graph::create_node(string type, string name) {
  if(name2id.count(name)) {
    show_error("node duplication", name);
  }
  nodes[type].insert(nnodes);
  name2id[name] = nnodes++;
}

void Graph::read(string filename) {
  ifstream f(filename);
  if(!f) {
    show_error("cannot open file", filename);
  }
  bool r = 1;
  string l, s;
  stringstream ss;
  vector<string> vs;
  while(1) {
    if(r) {
      if(!getline(f, l)) {
	break;
      }
      vs.clear();
      ss.str(l);
      ss.clear();
      while(getline(ss, s, ' ')) {
	vs.push_back(s);
      }
      if(vs.empty()) {
	continue;
      }
    }
    r = 1;
    if(vs[0][0] != '.') {
      show_error("unexpected line", l);
    }
    string type = vs[0].substr(1);
    if(vs.size() != 1) {
      for(int i = 1; i < (int)vs.size(); i++) {
	create_node(type, vs[i]);
      }
      continue;
    }
    std::vector<std::tuple<std::set<int>, std::set<int>, int> > v;
    while(getline(f, l)) {
      vs.clear();
      ss.str(l);
      ss.clear();
      while(getline(ss, s, ' ')) {
	vs.push_back(s);
      }
      if(vs.empty()) {
	continue;
      }
      if(vs[0][0] == '.') {
	r = 0;
	break;
      }
      int i = 0;
      set<int> senders;
      for(; i < (int)vs.size() && vs[i] != "->"; i++) {
	if(!name2id.count(vs[i])) {
	  show_error("unspecified node", vs[i]);
	}
	senders.insert(name2id[vs[i]]);	
      }
      if(i == (int)vs.size()) {
	show_error("incomplete line", l);
      }
      i++;
      set<int> recipients;
      for(; i < (int)vs.size() && vs[i] != ":"; i++) {
	if(!name2id.count(vs[i])) {
	  show_error("unspecified node", vs[i]);
	}
	recipients.insert(name2id[vs[i]]);
      }
      int band = 0;
      if(i < (int)vs.size() && vs[i] == ":") {
	i++;
	if(i == (int)vs.size()) {
	  show_error("incomplete line", l);
	}
	try {
	  band = str2int(vs[i]);
	}
	catch(...) {
	  show_error("non-integer weight", vs[i]);
	}
      }
      auto e = make_tuple(senders, recipients, band);
      v.push_back(e);
    }
    edges[type].insert(edges[type].end(), v.begin(), v.end());
  }
  
  f.close();
}

void Graph::print() {
  cout << "id to node name :" << endl;
  for(auto i : name2id) {
    cout << i.second << " : " << i.first << endl;;
  }
  for(auto &i : nodes) {
    cout << i.first << " :" << endl;
    for(auto j : i.second) {
      cout << j << ", ";
    }
    cout << endl;
  }
  for(auto &i : edges) {
    cout << i.first << " :" << endl;    
    for(auto &h : i.second) {
      for(int j : get<0>(h)) {
	cout << j << " ";
      }
      cout << "-> ";
      for(int j : get<1>(h)) {
	cout << j << " ";
      }
      if(get<2>(h) > 0) {
	cout << ": " << get<2>(h);
      }
      cout << endl;
    }
  }
}
