#ifndef CNF_HPP
#define CNF_HPP

#include <vector>
#include <set>

#include "utils/System.h"
#include "utils/ParseUtils.h"
#include "utils/Options.h"
#include "core/Dimacs.h"
#include "simp/SimpSolver.h"

void gen_cnf(Glucose::SimpSolver &S, int ncycle, int nnodes, int ndata, int ninputs, std::vector<int> &i_nodes, std::vector<int> &o_nodes, std::vector<int> &pe_nodes, std::vector<std::set<int> > &cons, std::vector<std::set<std::set<int> > > &operands, std::set<int> &output_ids, int fexmem);

#endif // CNF_HPP
