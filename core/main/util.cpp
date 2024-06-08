#include "util.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iomanip>
#include <sstream>

const size_t HEX_BASE_SIZE = sizeof (hex_base) * 8;
const size_t HEX_BASE_SIZE_SHIFTED = HEX_BASE_SIZE - 4;

hex_array convert::hexString_toBiner (const char *hex) {
  hex_array result;
  if (hex) { // hex not nullptr
    {
      size_t temp = strlen (hex);
      size_t a = temp & 0x7;
    	temp >>= 3;
      if (a) {
      	++temp;
      }
      result.reserve (temp > 1 ? temp : 1);
    }
    hex_base h_b = 0, t;
    while (*hex) { // while char '\0' stop iteration
      char h = *(hex++);
      if (h >= '0' && h <= '9') {
        h_b = h - '0';
      } else if (h >= 'a' && h <= 'f') {
        h_b = h - 'a' + 10;
      } else {
        // is invalid hex? but just let it
        continue;
      }
      for (hex_base& c : result) {
        t = (c >> HEX_BASE_SIZE_SHIFTED) & 0xf;
        c <<= 4;
        c |= h_b;
        h_b = t;
      }
      if (h_b) {
        result.emplace (result.begin(), h_b);
      }
    }
  }
  return result;
}
hex_array convert::hexString_toBiner (const std::string hex) {
  return convert::hexString_toBiner (hex.c_str ());
}

std::string convert::hexBiner_toString (const hex_array h) {
  std::ostringstream oss;
  for (hex_array::const_reverse_iterator i = h.crbegin (); i != h.crend (); ++i) {
    oss << std::hex << std::setw(8) << std::setfill('0') << *i;
  }
  return oss.str ();
}
std::ostream& operator<<(std::ostream& os, const hex_array& h) {
  for (hex_array::const_reverse_iterator i = h.crbegin(); i != h.crend(); ++i)
    os << std::hex << std::setw(8) << std::setfill('0') << *i;
  }
  return os;
}

// parsing json foward

// parsing json foward end

// parsing json extras
