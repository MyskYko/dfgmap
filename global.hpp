#ifndef GLOBAL_HPP
#define GLOBAL_HPP

#include <iostream>
#include <string>

static inline void show_error(std::string s) {
  std::cout << "error : " << s << std::endl;
  std::cout << "see usage by using option -h" << std::endl;
  abort();
}

#endif // GLOBAL_HPP
