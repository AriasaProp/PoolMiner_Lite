#include <cassert>
#include <iostream>

extern bool util_test();
extern bool json_test();

int main () {

  std::cout << "Begin Test!" << std::endl;
  assert(util_test ());
  assert(json_test ());
  std::cout << "End Test!" << std::endl;
  
  return 0;
}
