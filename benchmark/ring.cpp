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
    e << " pe" << i+1;
  }
  e << endl;
  e << ".com" << endl;
  for(int i = 0; i < N-1; i++) {
    e << "pe" << i+1 << " -> pe" << i+2 << " : 1" << endl;
  }
  e << "pe" << N << " -> pe" << 1 << " : 1" << endl;
  for(int i = 0; i < N; i++) {
    e << "_extmem -> pe" << i+1 << " : 1" << endl;
  }
  for(int i = 0; i < N; i++) {
    e << "pe" << i+1 << " -> _extmem : 1" << endl;
  }
  e.close();
    
  ofstream f("f.txt");
  f << ".i";
  for(int i = 0; i < N; i++) {
    f << " x" << i+1;
  }
  for(int i = 0; i < N; i++) {
    for(int j = 0; j < N; j++) {
      f << " w" << i+1 << "_" << j+1;
    }
  }
  f << endl;
  f << ".o";
  for(int i = 0; i < N; i++) {
    f << " y" << i+1;
  }
  f << endl;
  f << ".f" << endl;
  f << "+ 2 c a" << endl;
  f << "* 2 c a" << endl;
  f << ".m" << endl;
  f << "+ _ * _ _" << endl;
  f << ".n" << endl;  
  for(int i = 0; i < N; i++) {
    f << "y" << i+1;
    for(int j = N-1; j > 0; j--) {
      f << " + * x" << j+1 << " w" << i+1 << "_" << j+1;
    }
    f << " * x" << 1 << " w" << i+1 << "_" << 1;
    f << endl;
  }
  f.close();

  remove("g.txt");

  return 0;
}
