#include <string>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <iostream>
#include <chrono>
#include "Logger.hh"

using namespace std::chrono;

int main() {
  std::string lyrics = "Row, row, row your boat Gently down the stream, Merrily merrily, merrily, merrily Life is but a dream";
  std::istringstream iss(lyrics);
  std::vector<std::string> lyric_vec(std::istream_iterator<std::string>{iss},
                                     std::istream_iterator<std::string>{});
  std::ofstream mystream("/tmp/logger1.log", std::ios::out);
  auto start1 = high_resolution_clock::now();
  for (auto& lyric : lyric_vec) {
    mystream << lyric << std::endl;
    mystream.flush();
  }
  mystream.close();
  std::cout
    << duration_cast<nanoseconds>(high_resolution_clock::now()-start1).count()
    << std::endl;

  matan::Logger bunchLogger("/tmp/logger2.log");
  auto start2 = high_resolution_clock::now();
  for (auto& lyric : lyric_vec) {
    bunchLogger << lyric << std::endl;
  }
  std::cout
    << duration_cast<nanoseconds>(high_resolution_clock::now()-start2).count()
    << std::endl;

  std::cout << "finish logger" << std::endl;
  return 0;
}
