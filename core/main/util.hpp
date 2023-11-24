#ifndef _UTIL_INCLUDE_
#define _UTIL_INCLUDE_

#include <cstdint>
#include <string>
#include <vector>

typedef unsigned int hex_base;
typedef std::vector<hex_base> hex_array;

namespace convert {
extern hex_array hexString_toBiner (const char *);
extern hex_array hexString_toBiner (const std::string);
} // namespace convert


#endif //_UTIL_INCLUDE_