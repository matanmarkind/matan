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

template<typename BigMap>
void printBigMap(const BigMap& bigMap) {
  for (const auto& kv : bigMap) {
    std::cout << "(" << kv.first << "," << *kv.second << ") ";
  }
  std::cout << std::endl;
}
void bigMapTest(){
  matan::BigMap<int, std::string> bigMap;
  bigMap.append(2, "b");
  printBigMap(bigMap);
  bigMap.append(std::pair<int, std::string>(4, "d"));
  printBigMap(bigMap);
  bigMap.append(3, "c");
  printBigMap(bigMap);
  bigMap.batchAppend(std::vector<int>({26, 6}), std::vector<std::string>({"z", "f"}));
  printBigMap(bigMap);
  bigMap.batchInsert(std::vector<std::pair<int, std::string>>({{7, "g"}, {8, "h"}, {1, "a"}, {9, "i"}, {10, "j"}}));
  printBigMap(bigMap);
  bigMap.insert(1, "aa");
  printBigMap(bigMap);
  bigMap.batchAppend(std::vector<std::pair<int, std::string>>({{12, "l"}, {11, "k"}}));
  printBigMap(bigMap);
  bigMap.batchInsert<false>(std::vector<int>({14, 1}), std::vector<std::string>({"n", "m"}));
  printBigMap(bigMap);
}

int main() {
  bigMapTest();
  return EXIT_SUCCESS;
}
