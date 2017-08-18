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

int main() {
  matan::ThreadPool tp;

  std::packaged_task<int()> task([](){ return 7; }); // wrap the function
  std::future<int> f1 = task.get_future();  // get a future
  tp.enqueue(task);
  std::cout << f1.get() << std::endl;

  for (int x = 0; x < 5000; ++x) {
    for (int i = 0; i < 1000; ++i) {
      tp.enqueue(work_proc, i);
    }

    std::cout << "wait" << std::endl;
    tp.waitFinished();
    std::cout << "wait finished" << std::endl;
  }

  return 0;
}
