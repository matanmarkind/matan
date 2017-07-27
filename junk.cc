#include <iostream>
#include <string>
#include <type_traits>

int main()
{
  std::cout << std::is_trivially_move_constructible<int>::value << " "
            << std::is_trivially_move_constructible<std::string>::value << " "
            << std::endl;
}

