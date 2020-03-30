#include <iostream>
#include <fstream>
#include <vector>

using namespace std;

int main(int argc, char ** argv) {
  int N = atoi(argv[1]);
  int M = atoi(argv[2]);

  vector<int> felem(N*N);
  for(int i = 0; i < N*N; i++) {
    if(M & (1 << i)) {
      felem[i] = 1;
    }
  }

  vector<int> fin(N);
  for(int i = 0; i < N; i++) {
    int count = 0;
    for(int j = 0; j < N; j++) {
      count += felem[i + j*N];
    }
    fin[i] = count;
  }
  
  vector<int> fout(N);
  for(int i = 0; i < N; i++) {
    int count = 0;
    for(int j = 0; j < N; j++) {
      count += felem[j + i*N];
    }
    fout[i] = count;
  }

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
      if(felem[j + i*N]) {
	f << " W" << i+1 << "_" << j+1;
      }
    }
  }
  for(int i = 0; i < N; i++) {
    for(int j = 0; j < N; j++) {
      if(fin[i]) {
	f << " X" << i+1 << "_" << j+1;
      }
    }
  }
  f << endl;
  f << ".o";
  for(int i = 0; i < N; i++) {
    for(int j = 0; j < N; j++) {
      if(fout[i]) {
	f << " A" << i+1 << "_" << j+1;
      }
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
      if(fout[i]) {
	f << "A" << i+1 << "_" << j+1;
	for(int k = 0; k < fout[i]-1; k++) {
	  f << " +";
	}
	for(int k = 0; k < N; k++) {
	  if(felem[k + i*N]) {
	    f << " * W" << i+1 << "_" << k+1 << " X" << k+1 << "_" << j+1;
	  }
	}
	f << endl;
      }
    }
  }
  f.close();

  ofstream g("g.txt");
  g << ".assign" << endl;
  g << "_extmem";
  for(int i = 0; i < N; i++) {
    for(int j = 0; j < N; j++) {
      if(fin[i]) {
	g << " X" << i+1 << "_" << j+1;
      }
    }
  }
  g << endl;
  for(int i_ = 0; i_ < N; i_++) {
    for(int j_ = 0; j_ < N; j_++) {
      g << "rom" << i_+1 << "_" << j_+1;
      g << " { 1";
      for(int i = 0; i < N; i++) {
	for(int j = 0; j < N; j++) {
	  if(felem[j + i*N]) {
	    g << " W" << i+1 << "_" << j+1;
	  }
	}
      }
      g << " }";
      g << endl;
    }
  }
  g.close();

  return 0;
}
