#include <iostream>
#include <string>
#include <random>
#include <algorithm>
#include "ThreadPool.hh"
#include "BigMap.hh"

// a cpu-busy task.
void work_proc(unsigned int n) {
  std::random_device rd;
  std::mt19937 rng(rd());

  // build a vector of random numbers
  std::vector<int> data;
  data.reserve(n);
  std::generate_n(std::back_inserter(data), data.capacity(),
                  [&]() { return rng(); });
  std::sort(data.begin(), data.end());
}

void threadPoolTest() {
  matan::ThreadPool tp;

  // run five batches of 100 items
  for (int x = 0; x < 5000; ++x) {
    // queue 100 work tasks
    for (int i = 0; i < 1000; ++i)
      tp.enqueue(work_proc, i);

    std::cout << "wait" << std::endl;
    tp.waitFinished();
    std::cout << "wait finishied" << std::endl;
    std::cout << tp.getProcessed() << '\n';
  }
}
