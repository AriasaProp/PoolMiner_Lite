#include <iostream>
#include <filesystem>
#include <cstdint>
#include <cstdlib>

namespace fs = std::filesystem;

extern bool sample_json_parser (fs::path);

int main (int argc, char **argv) {
  bool passed = true;
  fs::path data_path = argv[1];
  passed &= sample_json_parser(data_path / "sample_json.json");

  return passed ? EXIT_SUCCESS : EXIT_FAILURE;
}
