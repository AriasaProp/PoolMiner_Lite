#include "json.hpp"
#include <iostream>

bool json_test () {
  std::string contents = R"({
    "String": "Hello, JSON!",
    "Integer": 42,
    "Float": 3.14,
    "Bool": true,
    "Array": [1, 2, 3, 4]
  })";
  json::JSON obj = json::Load (contents);
  std::string op1 = obj["String"];
  std::cout << "String : " << op1 << std::endl;
  long op2 = obj["Integer"];
  std::cout << "Integer : " << op2 << std::endl;
  double op3 = obj["Float"];
  std::cout << "Float : " << op3 << std::endl;
  bool op4 = obj["Bool"];
  std::cout << "Bool : " << op4 << std::endl;
  std::string op5 = obj["Array"];
  std::cout << "Array as String : " << op5 << std::endl;

  std::cout << "Output : " << obj << std::endl;
  return op4;
}
