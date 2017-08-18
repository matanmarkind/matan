#include "BunchQueue.hh"
#include <iostream>
#include <thread>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <chrono>
#include <type_traits>

struct S {
//  S(S& s) = delete;
//  S(S&& s) = delete;
  char c[2] = {0, 0};
};

void trivialVecQueue() {
  std::cout << std::is_trivially_move_constructible<S>::value << std::endl;
  S s1, s2;
  s1.c[0] = 'a';
  s2.c[0] = 'b';
  matan::VecQueue<S> v;
  v.push_back(s1);
  v.push_back(s2);
  v.push_back(std::move(s2));
  for (auto& ele : v) {
    std::cout << ele.c << "  ";
  }
  std::cout << s1.c << "  " << s2.c  << std::endl;
}


void nonTrivialVecQueue() {
  std::cout << std::is_trivially_move_constructible<std::string>::value << std::endl;
  std::string s = "hello, there";
  matan::VecQueue<std::string> v;
  v.push_back(s);
  v.emplace_back("hello, world, I have a fantastic surprice for you.");
  v.push_back(std::move(s));
  for (auto& ele : v) {
    std::cout << ele << "  ";
  }
  std::cout  << s << std::endl;
}

void trivialBunchQueue() {
  std::vector<int> nums = {1, 2, 3, 4, 5};
  matan::BunchQueue<int> queue;
  auto pusher = [&]() {
    for (int i : nums) {
      queue.push_back(i);
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  };
  auto printer = [&]() {
    while (!queue.empty()) {
      for (int i : queue.takeQueue()) {
        std::cout << i << " ";
      }
      std::cout << std::endl;
      std::this_thread::sleep_for(std::chrono::seconds(3));
    }
  };
    
  std::thread t1(pusher);
  std::this_thread::sleep_for(std::chrono::seconds(1));
  std::thread t2(pusher);
  std::this_thread::sleep_for(std::chrono::seconds(1));
  std::thread t3(printer);

  t1.join();
  t2.join();
  t3.join();
}

void nonTrivialBunchQueue() {
  std::string lyrics = "Row, row, row your boat Gently down the stream, Merrily merrily, merrily, merrily Life is but a dream";
  std::istringstream iss(lyrics);
  std::vector<std::string> lyric_vec(std::istream_iterator<std::string>{iss},
                                     std::istream_iterator<std::string>{});
  matan::BunchQueue<std::string> msgq(1);
  auto lyricist = [&msgq, &lyric_vec]() {
    for (const auto& lyric : lyric_vec) {
      msgq.push_back(lyric);
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  };
  auto singer = [&msgq]() {
    while (!msgq.empty()) {
      for (const auto& lyric : msgq.takeQueue()) {
        std::cout << lyric.c_str() << " ";
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

}

int main() {
  trivialVecQueue();
  std::cout << "trivialVecQueue passed" << std::endl;
  nonTrivialVecQueue();
  std::cout << "nonTrivialVecQueue passed" << std::endl;
  trivialBunchQueue();
  std::cout << "trivialBunchQueue passed" << std::endl;
  nonTrivialBunchQueue();
  std::cout << "nonTrivialBunchQueue passed" << std::endl;
  return 0;
}
