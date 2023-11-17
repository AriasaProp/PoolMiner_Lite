#include "json.hpp"

static json::JSON parse_next(const std::string &, size_t & );
static json::JSON parse_object(const std::string &str, size_t &offset ) {
    json::JSON Object = json::JSON::Make( json::JSON::Class::Object );

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

    return std::move( Object );
}
static json::JSON parse_array(const std::string &str, size_t &offset ) {
    json::JSON Array = json::JSON::Make( json::JSON::Class::Array );
    unsigned index = 0;
    
    do {
      ++offset;
    } while(isspace(str[offset]));
    if( str[offset] == ']' ) {
        ++offset; return std::move( Array );
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
            return std::move( json::JSON::Make( json::JSON::Class::Array ) );
        }
    }

    return std::move( Array );
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
                        return std::move( json::JSON::Make( json::JSON::Class::String ) );
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
    return std::move( String );
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
                return std::move( json::JSON::Make( json::JSON::Class::Null ) );
            }
            else
                break;
        }
        exp = std::stol( exp_str );
    }
    else if( !isspace( c ) && c != ',' && c != ']' && c != '}' ) {
        std::cerr << "ERROR: Number: unexpected character '" << c << "'\n";
        return std::move( json::JSON::Make( json::JSON::Class::Null ) );
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
    return std::move( Number );
}
static json::JSON parse_bool(const std::string &str, size_t &offset ) {
    json::JSON Bool;
    if( str.substr( offset, 4 ) == "true" )
        Bool = true;
    else if( str.substr( offset, 5 ) == "false" )
        Bool = false;
    else {
        std::cerr << "ERROR: Bool: Expected 'true' or 'false', found '" << str.substr( offset, 5 ) << "'\n";
        return std::move( json::JSON::Make( json::JSON::Class::Null ) );
    }
    offset += ((bool)Bool ? 4 : 5);
    return std::move( Bool );
}
static json::JSON parse_null(const std::string &str, size_t &offset ) {
    json::JSON Null;
    if( str.substr( offset, 4 ) != "null" ) {
        std::cerr << "ERROR: Null: Expected 'null', found '" << str.substr( offset, 4 ) << "'\n";
        return std::move( json::JSON::Make( json::JSON::Class::Null ) );
    }
    offset += 4;
    return std::move( Null );
}
static json::JSON parse_next(const std::string &str, size_t &offset ) {
    char value;
    while( isspace( str[offset] ) ) ++offset;
    value = str[offset];
    switch( value ) {
        case '[' : return std::move( parse_array( str, offset ) );
        case '{' : return std::move( parse_object( str, offset ) );
        case '\"': return std::move( parse_string( str, offset ) );
        case 't' :
        case 'f' : return std::move( parse_bool( str, offset ) );
        case 'n' : return std::move( parse_null( str, offset ) );
        default  : if( ( value <= '9' && value >= '0' ) || value == '-' )
                       return std::move( parse_number( str, offset ) );
    }
    std::cerr << "ERROR: Parse: Unknown starting character '" << value << "'\n";
    return json::JSON();
}
json::JSON json::JSON::Load(const std::string &str ) {
  size_t offset = 0;
  return std::move( parse_next( str, offset ) );
}
json::JSON json::Array() {
    return std::move( json::JSON::Make( json::JSON::Class::Array ) );
}

template <typename... T>
json::JSON json::Array( T... args ) {
    json::JSON arr = json::JSON::Make( json::JSON::Class::Array );
    arr.append( args... );
    return std::move( arr );
}

json::JSON json::Object() {
    return std::move( json::JSON::Make( json::JSON::Class::Object ) );
}

std::ostream& json::operator<<( std::ostream &os, const json::JSON &json ) {
    os << json.dump();
    return os;
}