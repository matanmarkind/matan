#include <iostream>
#include <string>
#include <random>
#include <algorithm>
#include "ThreadPool.hh"
#include <future>

// a cpu-busy task.
void work_proc(int n) {
  std::random_device rd;
  std::mt19937 rng(rd());

  // build a vector of random numbers
  std::vector<int> data;
  data.reserve(n);
  std::generate_n(std::back_inserter(data), data.capacity(),
                  [&]() { return rng(); });
  std::sort(data.begin(), data.end());
}

int small(int i) {
  return i;
}

int main() {
  matan::ThreadPool tp;

  auto f1 = tp.push_back_get_future(small, 7);
  
  for (int x = 0; x < 5; ++x) {
    for (int i = 0; i < 1000; ++i) {
      tp.push_back(work_proc, i);
    }

    std::cout << "wait" << std::endl;
    tp.waitFinished();
    std::cout << "wait finished" << std::endl;
  }
  
  std::cout << f1.get() << std::endl;

  return 0;
}
