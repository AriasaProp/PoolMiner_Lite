#include "util.hpp"

const size_t HEX_BASE_SIZE = sizeof(hex_base) * 8;

hex_array convert::hexString_toBiner (const char *hex) {
	hex_array result;
	if (hex) {//hex not nullptr
		while (*hex) { //while char '\0' stop iteration
			char h = *(hex++);
			hex_base h_b = 0;
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
				hex_base t = (c >> (HEX_BASE_SIZE - 4)) & 0xf;
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

//parsing json foward

//parsing json foward end


//parsing json extras



