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
  /*
  typedef std::pair<int, std::string&> KV;
  std::vector<KV> keys;
  keys.reserve(8);
  std::vector<std::string> vals;
  vals.reserve(8);
  vals.push_back("a");
  keys.push_back(KV(1, vals.back()));
  vals.push_back("c");
  keys.push_back(KV(3, vals.back()));
  vals.push_back("d");
  keys.push_back(KV(4, vals.back()));
  vals.push_back("b");
  keys.push_back(KV(2, vals.back()));
  vals.push_back("e");
  keys.push_back(KV(5, vals.back()));
  vals.push_back("z");
  keys.push_back(KV(26, vals.back()));
  vals.push_back("f");
  keys.push_back(KV(6, vals.back()));
  vals.push_back("g");
  keys.push_back(KV(7, vals.back()));
  std::cout << std::endl;
  for (auto s : keys) {
    std::cout << "(" << s.first << "," << s.second << ") ";
  }
  std::cout << std::endl;
  std::sort(keys.begin(),
                 keys.end(),
                 [](const KV& a, const KV& b) {
                   return a.first < b.first;
                 });
  for (auto s : keys) {
    std::cout << "(" << s.first << "," << s.second << ") ";
  }
  std::cout << std::endl;
   */
  return EXIT_SUCCESS;
}
