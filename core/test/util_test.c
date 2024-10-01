#include "util.h"
#include <iostream>

bool util_test () {
  std::cout << "HexArray Test Start" << std::endl;
  {
  	
  }
  std::cout << "HexArray Test Ended" << std::endl;

  std::cout << "Hashing Test Start" << std::endl;
  {
		const char *header = "01000000f615f7ce3b4fc6b8f61e8f89aedb1d0852507650533a9e3b10b9bbcc30639f279fcaa86746e1ef52d3edb3c4ad8259920d509bd073605c9bf1d59983752a6b06b817bb4ea78e011d012d59d4";
	  const char *expected = "d9eb8663ffec241c2fb118adb7de97a82c803b6ff46d57667935c81001000000";
	  hex_array refHeader = header;
	  hex_array refResult = hashN(refHeader);
	  hex_array refExpected = expected;
	  
	  
	  std::cout << "Header  : " << refHeader << std::endl;
	  std::cout << "Result  : " << refResult << std::endl;
	  std::cout << "Res 01  : 485eca4d2ed8962a05c3242866a7fbcb6dae3047d57a324b2a6e4059dbb88c59" << std::endl;
	  std::cout << "Expected: " << refExpected << std::endl;
  }
  std::cout << "Hashing Test Ended!" << std::endl;

	return true;
}