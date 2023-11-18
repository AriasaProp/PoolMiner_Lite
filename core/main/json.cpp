#include "json.hpp"

//define struct
json::JSON::JSON (): Internal (), Type (json::JSON::Class::Null) {}
json::JSON::JSON (std::initializer_list<json::JSON> list): json::JSON () {
  SetType (json::JSON::Class::Object);
  for (auto i = list.begin (), e = list.end (); i != e; ++i, ++i)
    operator[] ((std::string)*i) = *std::next (i);
}
json::JSON::JSON (json::JSON &&other): Internal (other.Internal), Type (other.Type) {
  other.Type = json::JSON::Class::Null;
  other.Internal.Map = nullptr;
}
json::JSON::JSON (const json::JSON &other) {
  switch (other.Type) {
  case json::JSON::Class::Object:
    Internal.Map = new std::unordered_map<std::string, json::JSON> (other.Internal.Map->begin (), other.Internal.Map->end ());
    break;
  case json::JSON::Class::Array:
    Internal.List = new std::deque<json::JSON> (other.Internal.List->begin (), other.Internal.List->end ());
    break;
  case json::JSON::Class::String:
    Internal.String = new std::string (*other.Internal.String);
    break;
  default:
    Internal = other.Internal;
  }
  Type = other.Type;
}
json::JSON& json::JSON::operator=(json::JSON &&other) {
  ClearInternal ();
  Internal = other.Internal;
  Type = other.Type;
  other.Internal.Map = nullptr;
  other.Type = json::JSON::Class::Null;
  return *this;
}
json::JSON& json::JSON::operator=(const json::JSON &other) {
    ClearInternal ();
    switch (other.Type) {
    case json::JSON::Class::Object:
      Internal.Map = new std::unordered_map<std::string, json::JSON> (other.Internal.Map->begin (), other.Internal.Map->end ());
      break;
    case json::JSON::Class::Array:
      Internal.List = new std::deque<json::JSON> (other.Internal.List->begin (), other.Internal.List->end ());
      break;
    case json::JSON::Class::String:
      Internal.String = new std::string (*other.Internal.String);
      break;
    default:
      Internal = other.Internal;
    }
    Type = other.Type;
    return *this;
  }
json::JSON::~JSON() {
  switch (Type) {
  case json::JSON::Class::Array:
    delete Internal.List;
    break;
  case json::JSON::Class::Object:
    delete Internal.Map;
    break;
  case json::JSON::Class::String:
    delete Internal.String;
    break;
  default:;
  }
}

static std::string json_escape (const std::string &str) {
  std::string output;
  for (unsigned i = 0; i < str.length (); ++i)
    switch (str[i]) {
    case '\"':
      output += "\\\"";
      break;
    case '\\':
      output += "\\\\";
      break;
    case '\b':
      output += "\\b";
      break;
    case '\f':
      output += "\\f";
      break;
    case '\n':
      output += "\\n";
      break;
    case '\r':
      output += "\\r";
      break;
    case '\t':
      output += "\\t";
      break;
    default:
      output += str[i];
      break;
    }
  return std::move (output);
}
json::JSON::operator std::string () const {
  return (Type == json::JSON::Class::String) ? std::move (json_escape (*Internal.String)) : std::string ("");
}
std::string json::JSON::dump (int depth, std::string tab) const {
    std::string pad = "";
    for (int i = 0; i < depth; ++i, pad += tab);

    switch (Type) {
    case json::JSON::Class::Null:
      return "null";
    case json::JSON::Class::Object: {
      std::string s = "{\n";
      bool skip = true;
      for (auto &p : *Internal.Map) {
        if (!skip) s += ",\n";
        s += (pad + "\"" + p.first + "\" : " + p.second.dump (depth + 1, tab));
        skip = false;
      }
      s += ("\n" + pad.erase (0, 2) + "}");
      return s;
    }
    case json::JSON::Class::Array: {
      std::string s = "[";
      bool skip = true;
      for (auto &p : *Internal.List) {
        if (!skip) s += ", ";
        s += p.dump (depth + 1, tab);
        skip = false;
      }
      s += "]";
      return s;
    }
    case json::JSON::Class::String:
      return "\"" + json_escape (*Internal.String) + "\"";
    case json::JSON::Class::Floating:
      return std::to_string (Internal.Float);
    case json::JSON::Class::Integral:
      return std::to_string (Internal.Int);
    case json::JSON::Class::Boolean:
      return Internal.Bool ? "true" : "false";
    default:
      return "";
    }
    return "";
  }


//extras
static json::JSON parse_next(const std::string &, size_t & );
static json::JSON parse_object(const std::string &str, size_t &offset ) {
    json::JSON Object = json::Make( json::JSON::Class::Object );

    do {
      ++offset;
    } while(isspace(str[offset]));
    if(str[offset] == '}') {
      ++offset;
      return std::move(Object);
    }

    while( true ) {
        json::JSON Key = parse_next( str, offset );
        while( isspace( str[offset] ) ) ++offset;
        if( str[offset] != ':' ) {
            std::cerr << "Error: Object: Expected colon, found '" << str[offset] << "'\n";
            break;
        }
        do {
          ++offset;
        } while(isspace(str[offset]));
        json::JSON Value = parse_next( str, offset );
        Object[(std::string)Key] = Value;
        
        while( isspace( str[offset] ) ) ++offset;
        if( str[offset] == ',' ) {
            ++offset; continue;
        }
        else if( str[offset] == '}' ) {
            ++offset; break;
        }
        else {
            std::cerr << "ERROR: Object: Expected comma, found '" << str[offset] << "'\n";
            break;
        }
    }

    return Object;
}
static json::JSON parse_array(const std::string &str, size_t &offset ) {
    json::JSON Array = json::Make( json::JSON::Class::Array );
    unsigned index = 0;
    
    do {
      ++offset;
    } while(isspace(str[offset]));
    if( str[offset] == ']' ) {
        ++offset; return Array;
    }

    while( true ) {
        Array[index++] = parse_next( str, offset );
        while( isspace( str[offset] ) ) ++offset;

        if( str[offset] == ',' ) {
            ++offset; continue;
        }
        else if( str[offset] == ']' ) {
            ++offset; break;
        }
        else {
            std::cerr << "ERROR: Array: Expected ',' or ']', found '" << str[offset] << "'\n";
            return json::Make( json::JSON::Class::Array);
        }
    }

    return Array;
}
static json::JSON parse_string(const std::string &str, size_t &offset ) {
    json::JSON String;
    std::string val;
    for( char c = str[++offset]; c != '\"' ; c = str[++offset] ) {
        if( c == '\\' ) {
            switch( str[ ++offset ] ) {
            case '\"': val += '\"'; break;
            case '\\': val += '\\'; break;
            case '/' : val += '/' ; break;
            case 'b' : val += '\b'; break;
            case 'f' : val += '\f'; break;
            case 'n' : val += '\n'; break;
            case 'r' : val += '\r'; break;
            case 't' : val += '\t'; break;
            case 'u' : {
                val += "\\u" ;
                for( unsigned i = 1; i <= 4; ++i ) {
                    c = str[offset+i];
                    if( (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F') )
                        val += c;
                    else {
                        std::cerr << "ERROR: String: Expected hex character in unicode escape, found '" << c << "'\n";
                        return json::Make( json::JSON::Class::String );
                    }
                }
                offset += 4;
            } break;
            default  : val += '\\'; break;
            }
        }
        else
            val += c;
    }
    ++offset;
    String = val;
    return String;
}
static json::JSON parse_number(const std::string &str, size_t &offset ) {
    json::JSON Number;
    std::string val, exp_str;
    char c;
    bool isDouble = false;
    long exp = 0;
    while( true ) {
        c = str[offset++];
        if( (c == '-') || (c >= '0' && c <= '9') )
            val += c;
        else if( c == '.' ) {
            val += c; 
            isDouble = true;
        }
        else
            break;
    }
    if( c == 'E' || c == 'e' ) {
        c = str[ offset++ ];
        if( c == '-' ){ ++offset; exp_str += '-';}
        while( true ) {
            c = str[ offset++ ];
            if( c >= '0' && c <= '9' )
                exp_str += c;
            else if( !isspace( c ) && c != ',' && c != ']' && c != '}' ) {
                std::cerr << "ERROR: Number: Expected a number for exponent, found '" << c << "'\n";
                return json::Make( json::JSON::Class::Null );
            }
            else
                break;
        }
        exp = std::stol( exp_str );
    }
    else if( !isspace( c ) && c != ',' && c != ']' && c != '}' ) {
        std::cerr << "ERROR: Number: unexpected character '" << c << "'\n";
        return json::Make( json::JSON::Class::Null );
    }
    --offset;
    
    if( isDouble )
        Number = std::stod( val ) * std::pow( 10, exp );
    else {
        if( !exp_str.empty() )
            Number = std::stol( val ) * std::pow( 10, exp );
        else
            Number = std::stol( val );
    }
    return Number;
}
static json::JSON parse_bool(const std::string &str, size_t &offset ) {
    json::JSON Bool;
    if( str.substr( offset, 4 ) == "true" )
        Bool = true;
    else if( str.substr( offset, 5 ) == "false" )
        Bool = false;
    else {
        std::cerr << "ERROR: Bool: Expected 'true' or 'false', found '" << str.substr( offset, 5 ) << "'\n";
        return json::Make( json::JSON::Class::Null );
    }
    offset += ((bool)Bool ? 4 : 5);
    return Bool;
}
static json::JSON parse_null(const std::string &str, size_t &offset ) {
    json::JSON Null;
    if( str.substr( offset, 4 ) != "null" ) {
        std::cerr << "ERROR: Null: Expected 'null', found '" << str.substr( offset, 4 ) << "'\n";
        return json::Make( json::JSON::Class::Null);
    }
    offset += 4;
    return Null;
}
static json::JSON parse_next(const std::string &str, size_t &offset ) {
    char value;
    while( isspace( str[offset] ) ) ++offset;
    value = str[offset];
    switch( value ) {
        case '[' : return parse_array( str, offset );
        case '{' : return parse_object( str, offset );
        case '\"': return parse_string( str, offset );
        case 't' :
        case 'f' : return parse_bool( str, offset );
        case 'n' : return parse_null( str, offset );
        default  : if( ( value <= '9' && value >= '0' ) || value == '-' )
                       return parse_number( str, offset );
    }
    std::cerr << "ERROR: Parse: Unknown starting character '" << value << "'\n";
    return json::JSON();
}

json::JSON json::Make (json::JSON::Class type) {
  JSON ret;
  ret.SetType (type);
  return ret;
}
json::JSON json::Parse(const std::string &str ) {
  size_t offset = 0;
  return parse_next( str, offset );
}

json::JSON json::Array() {
    return json::Make( json::JSON::Class::Array );
}

template <typename... T>
json::JSON json::Array( T... args ) {
    json::JSON arr = json::Make( json::JSON::Class::Array );
    arr.append( args... );
    return arr;
}

json::JSON json::Object() {
    return json::Make( json::JSON::Class::Object );
}

std::ostream& json::operator<<( std::ostream &os, const json::JSON &json ) {
    os << json.dump(1, "  ");
    return os;
}