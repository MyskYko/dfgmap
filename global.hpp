#ifndef GLOBAL_HPP
#define GLOBAL_HPP

#include <iostream>
#include <string>
#include <functional>
#include <algorithm>
#include <cctype>

static inline void show_error(std::string s) {
  std::cout << "error : " << s << std::endl;
  std::cout << "see usage by using option -h" << std::endl;
  abort();
}

static inline void show_error(std::string s, std::string s2) {
  std::cout << "error : " << s << " \"" << s2 << "\"" << std::endl;
  std::cout << "see usage by using option -h" << std::endl;
  abort();
}

static inline int str2int(std::string s) {
  if(!std::all_of(s.cbegin(), s.cend(), isdigit)) {
    throw "non-number included";
  }
  return std::stoi(s);
}

static void recursive_comb(int *indices, int s, int rest, std::function<void(int *)> f) {
  if(rest == 0) {
    f(indices);
  }
  else{
    if(s < 0) {
      return;
    }
    recursive_comb(indices, s - 1, rest, f);
    indices[rest - 1] = s;
    recursive_comb(indices, s - 1, rest - 1, f);
  }
}

static inline void foreach_comb(int n, int k, std::function<void(int *)> f) {
  int indices[k];
  recursive_comb(indices, n - 1, k, f);
}

static inline void foreach_perm(int n, std::function<void(int *)> f) {
  int indices[n];
  for(int i = 0; i < n; i++) {
    indices[i] = i;
  }
  do {
    f(indices);
  } while (std::next_permutation(indices, indices + n));
}

static inline int integer_log2(int n) { // wrap up
  int t = 1;
  int count = 0;
  while(n > t) {
    t = t << 1;
    count++;
  }
  return count;
}

static inline int integer_root(int n) { // wrap down
  if(n == 1) {
    return 1;
  }
  int s = n * n;
  int l = n/2;
  int r = n;
  while(r > l + 1) {
    int m = (l + r) / 2;
    if(m * m > s) {
      r = m;
    }
    else if(m * m < s) {
      l = m;
    }
    else {
      return m;
    }
  }
  if(r * r == s) {
    return r;  
  }
  return l;
}

#endif // GLOBAL_HPP
