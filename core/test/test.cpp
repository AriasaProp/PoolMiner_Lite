#include <iostream>
#include <filesystem>
#include <cstdint>
#include <cstdlib>

namespace fs = std::filesystem;

extern bool miner_test (fs::path);

int main (int argc, char **argv) {
  bool passed = true;
  fs::path data_path = argv[1];
  passed &= miner_test(data_path / "sample_json.json");

  return passed ? EXIT_SUCCESS : EXIT_FAILURE;
}
