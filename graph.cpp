#include <fstream>
#include <sstream>

#include "global.hpp"
#include "graph.hpp"

using namespace std;

void Graph::create_node(string type, string name) {
  if(name2id.count(name)) {
    show_error("node duplication : " + name);
  }
  nodes[type].insert(nnodes);
  name2id[name] = nnodes++;
}

void Graph::read(string filename) {
  ifstream f(filename);
  if(!f) {
    show_error("cannot open file : " + filename);
  }

  int r = 1;
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
      show_error("line without type : " + l);
    }
    string type = vs[0].substr(1);
    if(vs.size() != 1) {
      for(int i = 1; i < vs.size(); i++) {
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
	r = 1;
	break;
      }
      int i = 0;
      set<int> senders;
      for(; i < vs.size() && vs[i] != "->"; i++) {
	if(!name2id.count(vs[i])) {
	  show_error("unspecified node " + vs[i]);
	}
	senders.insert(name2id[vs[i]]);	
      }
      if(i == vs.size()) {
	show_error("incomplete line : " + l);
      }
      i++;
      set<int> recipients;
      for(; i < vs.size() && vs[i] != ":"; i++) {
	if(!name2id.count(vs[i])) {
	  show_error("unspecified node : " + vs[i]);
	}
	recipients.insert(name2id[vs[i]]);
      }
      int band = -1;
      if(i < vs.size() && vs[i] == ":") {
	i++;
	if(i == vs.size()) {
	  show_error("incomplete line : " + l);
	}
	try {
	  band = stoi(vs[i]);
	}
	catch(...) {
	  show_error("bandwidth must be integer : " + vs[i]);
	}
	if(band <= 0) {
	  show_error("bandwidth must be more than 0 : " + vs[i]);
	}
      }
      auto e = make_tuple(senders, recipients, band);
      v.push_back(e);
    }
    edges[type].insert(edges[type].end(), v.begin(), v.end());
  }
  
  f.close();
}
