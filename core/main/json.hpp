#include <string>

namespace json {
	enum value_type: char {
		Unknown = 0,
		Integer,
		Object,
		Array
	};
	struct value {
		virtual value_type getType() const;
	};
	
	struct json_integer: public value {
		json_integer(int);
		operator int() const;
		value_type getType() override const;
	};
	struct json_object: public value {
		
		value *operator[] (std::string)
		value_type getType() override const;
	};
	struct json_array: public value {
		
		value_type getType() override const;
	};
}