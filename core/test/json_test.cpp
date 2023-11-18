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
  json_resource obj (contents);
  std::string op1 = obj["String"].as_str();
  std::cout << "String : " << op1 << std::endl;
  long op2 = obj["Integer"].as<int>();
  std::cout << "Integer : " << op2 << std::endl;
  double op3 = obj["Float"].as<float>();
  std::cout << "Float : " << op3 << std::endl;
  bool op4 = obj["Bool"].as<bool>();
  std::cout << "Bool : " << op4 << std::endl;
  std::cout << "Array : [";
  std::cout << obj["Array"][0].as<int>();
  std::cout << ", ";
  std::cout << obj["Array"][1].as<int>();
  std::cout << ", ";
  std::cout << obj["Array"][2].as<int>();
  std::cout << ", ";
  std::cout << obj["Array"][3].as<int>();
  std::cout << "] " << std::endl;

  std::cout << "Output : " << obj.as_str() << std::endl;
  
  return op4;
}
