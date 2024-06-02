#ifndef _UTIL_INCLUDE_
#define _UTIL_INCLUDE_

#include <string>
#include <vector>

#if defined(_MSC_VER) && (_MSC_VER < 1600)
typedef signed __int16 int16_t;
typedef unsigned __int16 uint16_t;
typedef signed __int32 int32_t;
typedef unsigned __int32 uint32_t;
#elif defined(__SYMBIAN32__)
typedef unsigned short uint16_t;
typedef signed short int16_t;
typedef unsigned int uint32_t;
typedef signed int int32_t;
#else
#include <cstdint>
#endif

// 32bit hex
typedef uint32_t hex_base;
typedef std::vector<hex_base> hex_array;

namespace convert {
hex_array hexString_toBiner (const char *);
hex_array hexString_toBiner (const std::string);
std::string hexBiner_toString (const hex_array);
} // namespace convert

#endif //_UTIL_INCLUDE_