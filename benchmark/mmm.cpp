#include <iostream>
#include <fstream>

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
  e << ".mem";
  for(int i = 0; i < N; i++) {
    for(int j = 0; j < N; j++) {
      e << " rom" << i+1 << "_" << j+1;
    }
  }
  e << endl;
  e << ".com" << endl;
  for(int i = 0; i < N; i++) {
    e << "_extmem -> pe" << i+1 << "_1 : 1" << endl;
  }
  for(int i = 0; i < N-1; i++) {
    for(int j = 0; j < N; j++) {
      e << "pe" << i+1 << "_" << j+1 << " -> pe" << i+2 << "_" << j+1 << " : 1" << endl;
    }
  }
  for(int i = 0; i < N; i++) {
    for(int j = 0; j < N-1; j++) {
      e << "pe" << i+1 << "_" << j+1 << " -> pe" << i+1 << "_" << j+2 << " : 1" << endl;
    }
  }
  for(int i = 0; i < N; i++) {
    for(int j = 0; j < N; j++) {
      e << "rom" << i+1 << "_" << j+1 << " -> pe" << i+1 << "_" << j+1 << endl;
    }
  }
  for(int j = 0; j < N; j++) {
    e << "pe" << N << "_" << j+1 << " -> _extmem : 1" << endl;
  }
  e.close();
    
  ofstream f("f.txt");
  f << ".i";
  for(int i = 0; i < N; i++) {
    for(int j = 0; j < N; j++) {
      f << " W" << i+1 << "_" << j+1;
    }
  }
  for(int i = 0; i < N; i++) {
    for(int j = 0; j < N; j++) {
      f << " X" << i+1 << "_" << j+1;
    }
  }
  f << endl;
  f << ".o";
  for(int i = 0; i < N; i++) {
    for(int j = 0; j < N; j++) {
      f << " A" << i+1 << "_" << j+1;
    }
  }
  f << endl;
  f << ".f" << endl;
  f << "+ 2 c a" << endl;
  f << "* 2 c a" << endl;
  f << ".m" << endl;
  f << "+ _ * _ _" << endl;
  f << ".n" << endl;  
  for(int i = 0; i < N; i++) {
    for(int j = 0; j < N; j++) {
      f << "A" << i+1 << "_" << j+1;
      for(int k = N-1; k > 0; k--) {
	f << " + * W" << i+1 << "_" << k+1 << " X" << k+1 << "_" << j+1;
      }
      f << " * W" << i+1 << "_" << 1 << " X" << 1 << "_" << j+1;
      f << endl;
    }
  }
  f.close();

  ofstream g("g.txt");
  g << ".assign" << endl;
  g << "_extmem";
  for(int i = 0; i < N; i++) {
    for(int j = 0; j < N; j++) {
      g << " X" << i+1 << "_" << j+1;
    }
  }
  g << endl;
  for(int i = 0; i < N; i++) {
    for(int j = 0; j < N; j++) {
      g << "rom" << j+1 << "_" << i+1 << " W" << i+1 << "_" << j+1 << endl;
    }
  }
  g.close();

  return 0;
}
