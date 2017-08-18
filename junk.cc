// Example program
#include <iostream>
#include <type_traits>
#include <string>
#include <vector>
#include "BunchQueue.hh"

struct S {
    int a;
    bool b;
    char c[9];
};


int main() {
  std::string s = "hello, there";
  matan::VecQueue<std::string> v;
  v.push_back(s);
  v.emplace_back("hello, world");
  v.push_back(std::move(s));
  std::cout << *v.begin() << "______" << s << std::endl;
  std::cout << std::is_trivially_copyable<S>::value << "    " << std::is_trivially_move_constructible<S>::value << std::endl;
}
