#include "MessageQueue.hh"
#include <iostream>
#include <thread>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <chrono>

int main() {
  std::string lyrics = "Row, row, row your boat Gently down the stream, Merrily merrily, merrily, merrily Life is but a dream";
  std::istringstream iss(lyrics);
  std::vector<std::string> lyric_vec(std::istream_iterator<std::string>{iss},
                                     std::istream_iterator<std::string>{});
  matan::MessageQueue<std::string> msgq(1);
  auto lyricist = [&msgq, &lyric_vec]() {
    for (const auto& lyric : lyric_vec) {
      msgq.push_back(lyric);
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  };
  auto singer = [&msgq]() {
    while (!msgq.empty()) {
      for (const auto& lyric : msgq.takeQueue()) {
        std::cout << lyric << " ";
      }
      std::cout << std::endl;
      std::this_thread::sleep_for(std::chrono::seconds(4));
    }
  };

  std::thread t1(lyricist);
  std::this_thread::sleep_for(std::chrono::seconds(4));
  std::thread t2(lyricist);
  std::this_thread::sleep_for(std::chrono::seconds(1));
  std::thread t3(singer);

  t1.join();
  t2.join();
  t3.join();
  return 0;
}
