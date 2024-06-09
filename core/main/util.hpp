#ifndef _UTIL_INCLUDE_
#define _UTIL_INCLUDE_

#include <string>
#include <vector>
#include <iostream>

#if defined(_MSC_VER) && (_MSC_VER < 1600)
typedef unsigned __int32 size_t;
typedef unsigned __int32 uint32_t;
typedef unsigned __int8 uint8_t;
#elif defined(__SYMBIAN32__)
typedef unsigned int size_t;
typedef unsigned int uint32_t;
typedef unsigned char uint8_t;
#else
#include <cstdint>
#endif

//hex Array
typedef uint32_t hex_base;
typedef std::vector<hex_base> hex_array;

namespace convert {
hex_array hexString_toBiner (const char *);
hex_array hexString_toBiner (const std::string);
std::string hexBiner_toString (const hex_array);
} // namespace convert
std::ostream& operator<<(std::ostream&, const hex_array&);


//hashing
#include "algorithm/sha256.hpp"

struct hashing {
private:
  size_t i, j, k, l;
  uint32_t xs[16];
  uint8_t B[132];
  uint32_t X[32];
  uint32_t V[32768];
  Sha256Context context;
  void innerHash ();

public:
  uint8_t H[SHA256_HASH_SIZE]; // cache of hashing
  hashing ();
  ~hashing ();
  void xorSalsa8 ();
  void xorSalsa8 (const size_t,const size_t);
  void hash (const uint8_t *, const uint32_t);
  void hash (const uint8_t *); //combined nonce
};
hex_array hashN (const hex_array&);

#endif //_UTIL_INCLUDE_