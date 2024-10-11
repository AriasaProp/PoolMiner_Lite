#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>
#include "miner_core.hpp"

namespace fs = std::filesystem;

bool sample_json_parser (fs::path f) {
	std::cout << "Sample Json Parser" << std::endl;
	miner::init();
  std::ifstream file(f, std::ios::binary | std::ios::ate);
  if (!file.is_open()) {
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
  	miner::parsing(buffer);
  } else {
    std::cerr << "Gagal membaca file!" << std::endl;
  }

  // Jangan lupa untuk membersihkan alokasi memori
  delete[] buffer;
  file.close();
  miner::clear();
  return true;
}

