#include <stdio.h>

extern int json_test();

int main(int argv, char **args) {

  int passed = 0;
  
  printf("JSON test start!");
  
  passed = json_test();
  
  return passed;
}
