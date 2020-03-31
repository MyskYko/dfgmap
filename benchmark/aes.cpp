#include <iostream>
#include <fstream>
#include <cstdio>

using namespace std;

int main(int argc, char ** argv) {
  if(argc < 2) {
    cout << "specify a parameter" << endl;
    return 1;
  }
  int N = atoi(argv[1]);

  ofstream e("e.txt");
  e << ".pe";
  for(int i = 0; i < N; i++) {
    for(int j = 0; j < N; j++) {
      e << " pe" << i+1 << "_" << j+1;
    }
  }
  e << endl;
  e << ".com" << endl;

  for(int i = 0; i < N; i++) {
    e << "_extmem -> pe1_" << i+1 << " : 1" << endl;
  }
  for(int i = 0; i < N-1; i++) {
    for(int j = 0; j < N; j++) {
      e << "pe" << i+1 << "_" << j+1 << " -> pe" << i+2 << "_" << j+1 << " : 1" << endl;
      e << "pe" << i+2 << "_" << j+1 << " -> pe" << i+1 << "_" << j+1 << " : 1" << endl;
    }
  }
  for(int i = 0; i < N; i++) {
    for(int j = 0; j < N-1; j++) {
      e << "pe" << i+1 << "_" << j+1 << " -> pe" << i+1 << "_" << j+2 << " : 1" << endl;
      e << "pe" << i+1 << "_" << j+2 << " -> pe" << i+1 << "_" << j+1 << " : 1" << endl;
    }
  }
  for(int j = 0; j < N; j++) {
    e << "pe" << N << "_" << j+1 << " -> _extmem : 1" << endl;
  }
  e.close();

  ofstream f("f.txt");
  f << ".i input" << endl;
  f << ".o output" << endl;
  f << ".f" << endl;
  f << "addshift0 1" << endl <<
    "keysub 1" << endl <<
    "keysche 1" << endl <<
    "sub0 1" << endl <<
    "sub1 1" << endl <<
    "sub2 1" << endl <<
    "sub3 1" << endl <<
    "mix0 1" << endl <<
    "mix1 1" << endl <<
    "mix2 1" << endl <<
    "mix3 1" << endl <<
    "merge0 2" << endl <<
    "merge1 2" << endl <<
    "merge2 2" << endl <<
    "addshift 2" << endl <<
    "addkey 2" << endl;
  f << ".n" << endl;
  f << "n_0_1 addshift0 input" << endl;
  for(int i = 0; i <= 9; i++) {
    if(i != 0) {
      f << "n_" << i << "_1 addshift n_" << i-1 << "_7 n_" << i-1 << "_14" << endl;      
    }
    f << "n_" << i << "_2 keysub n_" << i << "_1" << endl <<
      "n_" << i << "_3 sub2 n_" << i << "_1" << endl <<
      "n_" << i << "_4 sub3 n_" << i << "_1" << endl <<
      "n_" << i << "_5 sub1 n_" << i << "_1" << endl <<
      "n_" << i << "_6 sub0 n_" << i << "_1" << endl <<
      "n_" << i << "_7 keysche n_" << i << "_2" << endl;
    if(i == 9) {
      break;
    }
    f << "n_" << i << "_8 mix2 n_" << i << "_3" << endl <<
      "n_" << i << "_9 mix3 n_" << i << "_4" << endl <<
      "n_" << i << "_10 mix1 n_" << i << "_5" << endl <<
      "n_" << i << "_11 mix0 n_" << i << "_6" << endl <<
      "n_" << i << "_12 merge1 n_" << i << "_8 n_" << i << "_9" << endl <<
      "n_" << i << "_13 merge0 n_" << i << "_10 n_" << i << "_11" << endl <<
      "n_" << i << "_14 merge2 n_" << i << "_12 n_" << i << "_13" << endl;
  }
  f << "n_9_8 merge1 n_9_3 n_9_4" << endl <<
    "n_9_9 merge0 n_9_5 n_9_6" << endl <<
    "n_9_10 merge2 n_9_8 n_9_9" << endl <<
    "output addkey n_9_7 n_9_10" << endl;
  
  f.close();

  remove("g.txt");

  return 0;
}
