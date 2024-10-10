#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;

bool sample_json_parser (fs::path f) {
	std::cout << "Sample Json Parser" << std::endl;
  std::ifstream file(f);
  if (!file.is_open()) {
    std::cerr << "Gagal membuka file!" << std::endl;
    return false;
  }

  std::string line;
  while (std::getline(file, line)) {
    std::cout << line << std::endl;
  }

  file.close();

  return true;
}

