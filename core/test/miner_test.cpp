#include "json.hpp"
#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>
#include "miner_core.hpp"
#include "jansson.hpp"

namespace fs = std::filesystem;

bool miner_test (fs::path f) {
	std::cout << "Sample Json Parser" << std::endl;
	miner::init();
  std::ifstream file(f, std::ios::binary | std::ios::ate);
  if (!file.is_open()) [[unlikely]] {
    std::cerr << "Gagal membuka file!" << std::endl;
    return false;
  }

  std::streamsize size = file.tellg();
  file.seekg(0, std::ios::beg);

  // Mengalokasikan buffer untuk menampung isi file
  char* buffer = new char[size + 1];  // +1 untuk karakter null-terminator
  buffer[size] = '\0';  // Menambahkan karakter null-terminator di akhir

  // Membaca isi file ke buffer
  if (file.read(buffer, size)) {
  	std::cout << miner::parsing(buffer) << std::endl;
  } else [[unlikely]] {
    std::cerr << "Gagal membaca file!" << std::endl;
  }

  // Jangan lupa untuk membersihkan alokasi memori
  delete[] buffer;
  file.close();
  miner::clear();
  return true;
}

