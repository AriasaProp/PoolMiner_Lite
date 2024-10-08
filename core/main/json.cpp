#include "json.hpp"

value_type json::value::getType() const { 
	return value_type::Unknown;
}


json::json_integer::json_integer(int i): value(i) {}
int json::json_integer::operator int() const { 
	return value;
}
value_type json::json_integer::getType() const { 
	return value_type::Integer;
}


value_type json::json_object::getType() const { 
	return value_type::Object;
}




value_type json::json_array::getType() const { 
	return value_type::Array;
}