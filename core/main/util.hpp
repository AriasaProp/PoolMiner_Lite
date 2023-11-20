#ifndef _UTIL_INCLUDE_
#define _UTIL_INCLUDE_

#include <cstdint>
#include <string>

namespace convert {
extern void hexString_toBiner (void *, const char *, const size_t);
extern void hexString_toBiner (void *, const std::string, const size_t);
} // namespace convert

namespace minerpool {
	
struct mine_data;
mine_data create_data();
extern bool mine_data_update(mine_data*, const char*);
extern std::string mine_data_extract(mine_data*);
}

#endif //_UTIL_INCLUDE_