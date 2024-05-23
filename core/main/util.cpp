#include "util.hpp"

#include <cmath>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cstring>

const size_t HEX_BASE_SIZE = sizeof(hex_base) * 8;
const size_t HEX_BASE_SIZE_SHIFTED = HEX_BASE_SIZE - 4;

hex_array convert::hexString_toBiner (const char *hex) {
	hex_array result;
	if (hex) {//hex not nullptr
		{
			double temp = double(strlen(hex));
			temp /= double(sizeof(hex_base) * 2.0);
			size_t reserve = static_cast<size_t>(ceil(temp));
			result.reserve(reserve>1?reserve:1);
		}
		hex_base h_b = 0, t;
		while (*hex) { //while char '\0' stop iteration
			char h = *(hex++);
			if (h >= '0' && h <= '9') {
				h_b = h - '0';
			} else if (h >= 'a' && h <= 'f') {
				h_b = h - 'a' + 10;
			} else {
				// is invalid hex? but just let it
				continue;
			}
			for (hex_array::iterator i = result.begin(); i < result.end(); ++i) {
				hex_base &c = *i;
				t = (c >> HEX_BASE_SIZE_SHIFTED) & 0xf;
				c <<= 4;
				c |= h_b;
				h_b = t;
			}
			if (h_b) {
				result.push_back(h_b);
			}
		}
	}
	return result;
}
hex_array convert::hexString_toBiner (const std::string hex) {
	return convert::hexString_toBiner(hex.c_str());
}

std::string convert::hexBiner_toString(const hex_array hex) {
  std::ostringstream oss;
  for (hex_array::const_reverse_iterator it = hex.rbegin(); it < hex.rend(); ++it) {
    oss << std::hex << *it;
  }
  return oss.str();
}


//parsing json foward

//parsing json foward end


//parsing json extras



