#include "algorithm/hashing.hpp"
#include "util.hpp"
#include <iostream>

bool mine_test () {
	const char *header = "01000000f615f7ce3b4fc6b8f61e8f89aedb1d0852507650533a9e3b10b9bbcc30639f279fcaa86746e1ef52d3edb3c4ad8259920d509bd073605c9bf1d59983752a6b06b817bb4ea78e011d012d59d4";
  const char *expected = "d9eb8663ffec241c2fb118adb7de97a82c803b6ff46d57667935c81001000000";

  std::cout << "Hashing Test Start" << std::endl;
  hex_array refHeader = convert::hexString_toBiner(header);
  std::cout << "Header : " << refHeader << std::endl;
  
  hex_array result;
  result.reserve(16);
  hashN((uint8_t*)refHeader.data(), (uint8_t*)result.data());
  std::cout << "Result  : " << result << std::endl;
  
  hex_array refExpected = convert::hexString_toBiner(expected);
  std::cout << "Expected: " << refExpected << std::endl;
  std::cout << "Hashing Test Ended!" << std::endl;

	return true;
}