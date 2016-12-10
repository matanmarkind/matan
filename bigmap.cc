#include "timsort.hh"
#include "BigMap.hh"

#include "iostream"

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
  std::cout << "remove 2 "; bigMap.remove(2);
  printBigMap(bigMap);
  bigMap.append(std::pair<int, std::string>(4, "d"));
  printBigMap(bigMap);
  bigMap.append(3, "c");
  printBigMap(bigMap);
  bigMap.batchAppend(std::vector<int>({26, 6}), std::vector<std::string>({"z", "f"}));
  printBigMap(bigMap);
  bigMap.batchInsert(std::vector<std::pair<int, std::string>>({{7, "g"}, {8, "h"}, {1, "a"}, {9, "i"}, {10, "j"}}));
  printBigMap(bigMap);
  std::cout << "remove 10 "; bigMap.remove(10);
  printBigMap(bigMap);
  bigMap.insert(1, "aa");
  printBigMap(bigMap);
  bigMap.batchAppend(std::vector<std::pair<int, std::string>>({{12, "l"}, {11, "k"}}));
  printBigMap(bigMap);
  std::cout << "remove 12 "; bigMap.remove(12);
  printBigMap(bigMap);
  bigMap.batchInsert(std::vector<int>({14, 1}), std::vector<std::string>({"n", "m"}));
  printBigMap(bigMap);
  bigMap.deepSort();
  std::cout << "remove 7 "; bigMap.remove(7);
  printBigMap(bigMap);
  bigMap.append(2, "b");
  printBigMap(bigMap);
  bigMap.deepSort();
  std::cout << "remove 4 "; bigMap.remove(4);
  printBigMap(bigMap);
}

int main() {
  bigMapTest();
  return EXIT_SUCCESS;
}
