#ifndef GLOBAL_HPP
#define GLOBAL_HPP

#include <iostream>
#include <string>
#include <functional>

static inline void show_error(std::string s) {
  std::cout << "error : " << s << std::endl;
  std::cout << "see usage by using option -h" << std::endl;
  abort();
}

static void recursive_comb(int *indexes, int s, int rest, std::function<void(int *)> f) {
  if (rest == 0) {
    f(indexes);
  } else {
    if (s < 0) return;
    recursive_comb(indexes, s - 1, rest, f);
    indexes[rest - 1] = s;
    recursive_comb(indexes, s - 1, rest - 1, f);
  }
}

static void foreach_comb(int n, int k, std::function<void(int *)> f) {
  int indexes[k];
  recursive_comb(indexes, n - 1, k, f);
}

static int integer_log2(int n) {
  int t = 1;
  int count = 0;
  while(n > t) {
    t = t << 1;
    count++;
  }
  return count;
}

#endif // GLOBAL_HPP
