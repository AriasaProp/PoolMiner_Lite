#ifndef _UTIL_INCLUDE_
#define _UTIL_INCLUDE_

#include <string>
#include <vector>
#include <iostream>


#if defined(_MSC_VER) && (_MSC_VER < 1600)
typedef unsigned __int32 uint32_t;
#elif defined(__SYMBIAN32__)
typedef unsigned int uint32_t;
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
std::ostream& operator<<(std::ostream&, const hex_array&);

#endif //_UTIL_INCLUDE_