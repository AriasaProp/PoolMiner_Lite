#include <cassert>
#include <iostream>

extern bool util_test ();
extern bool json_test ();
extern bool mine_test ();

int main () {

  std::cout << "Begin Test!" << std::endl;
  assert (json_test ());
  assert (util_test ());
  std::cout << "End Test!" << std::endl;

  return 0;
}
