#include "json.hpp"

inline 
json_resource::~json_resource (){
    if (parsed_data_p) delete parsed_data_p;
}

inline 
json_resource::json_resource (const json_resource& r) {
    data=r.data;
    _exists = r._exists;
    if(r.parsed_data_p) parsed_data_p = new json_parsedData(*(r.parsed_data_p));
    else parsed_data_p = NULL;
}

inline 
json_resource& json_resource::operator= (const json_resource& r) {
    data=r.data;
    _exists = r._exists;
    if(r.parsed_data_p) parsed_data_p = new json_parsedData(*(r.parsed_data_p));
    else parsed_data_p = NULL;
    return *this;
}

inline 
int json_resource::size (void) {
    if (!exists()) return (0);
    parse(); // parse if not parsed
    return (parsed_data_p->size());
}

inline 
json_resourceType json_resource::type (void) {
    if (!exists()) return (JSON_UNINITIATED);
    parse(); // parse if not parsed
    return (parsed_data_p->type);
}

inline 
std::string json_resource::as_str (bool print_comments, bool update_data) {
    if (exists()) {
        std::string ret;
        parse(); // parse if not parsed
        parsed_data_p->cleanup();
        
        if (parsed_data_p->type==JSON_OBJECT) {
            ret = "{\n";
            for (auto it=parsed_data_p->object.begin(); it!=parsed_data_p->object.end(); ++it) {
                ret += json_printtab + "'" + it->first + "': " + insert_tab_after_newlines( it->second.as_str (print_comments, update_data) );
                if (std::next(it) != parsed_data_p->object.end()) ret += ",";
                if (print_comments)
                    ret += " // " + to_string(it->second.type());
                ret += "\n";
            }
            ret += "}";
        }
        else if (parsed_data_p->type==JSON_ARRAY) {
            ret = "[\n";
            for (auto it=parsed_data_p->array.begin(); it!=parsed_data_p->array.end(); ++it) {
                ret += json_printtab + insert_tab_after_newlines( it->as_str (print_comments, update_data) );
                if (std::next(it) != parsed_data_p->array.end()) ret += ",";
                if (print_comments)
                    ret += " // " + to_string(it->type());
                ret += "\n";
            }
            ret += "]";
        }
        else // JSON_LEAF or JSON_UNKNOWN
             ret = strtrim (data);
        
        if (update_data) data = ret;
        return (ret);
    }
    else
        return ("");
}

// Parsers

inline 
json_resourceType json_resource::parse (bool force) {
    if (!parsed_data_p)  parsed_data_p = new json_parsedData;
    if (parsed_data_p->type==JSON_UNKNOWN || force)  parsed_data_p->parse (data, JSON_UNKNOWN);
    return (parsed_data_p->type);
}

inline 
void json_resource::parse_full (bool force, int max_depth, int* parse_count_for_verbose_p) {
	// recursive parsing (slow)
    if (max_depth==0) return;
    if (!parsed_data_p)  parsed_data_p = new json_parsedData;
    if (parsed_data_p->type==JSON_UNKNOWN || force)  parsed_data_p->parse (data, JSON_UNKNOWN);
    // verbose
    if (parse_count_for_verbose_p) {
        (*parse_count_for_verbose_p)++;
        if ( (*parse_count_for_verbose_p) % 100 == 0)
            std::cout << "parse_full: " << (*parse_count_for_verbose_p) << " calls." << std::endl;
    }
    // recursive parse children if not already parsed
    if (parsed_data_p->type==JSON_OBJECT) 
        for (auto it=parsed_data_p->object.begin(); it!=parsed_data_p->object.end(); ++it)
            it->second.parse_full (force, max_depth-1, parse_count_for_verbose_p);
    else if (parsed_data_p->type==JSON_ARRAY)
        for (auto it=parsed_data_p->array.begin(); it!=parsed_data_p->array.end(); ++it) 
            it->parse_full (force, max_depth-1, parse_count_for_verbose_p);
}

// ------------------------------------------------------------
// ============================================================
// FAST PARSER (Under construction. DO NOT use the following functions in your application.)

inline 
int seek_next (std::string* str_p, int start_pos, char character) {
    return 0;
}

inline 
void json_resource::fast_parse (std::string* str_p, bool copy_string, int max_depth, int* parse_start_str_pos) {
    // TODO: UNDER CONSTRUCTION...
    
    if (!str_p)
        str_p = &data;
    std::string& str = *str_p;
    
    // splits, while respecting brackets and escapes
    //std::vector<std::string> ret;
    
    //std::string current;
    std::vector<int> bracket_stack;
    std::vector<int> quote_stack;
    bool escape_active = false;
    int bi;
    
    bool initial_whitespaces = true;
    bool isroot = false;
    
    if (!parse_start_str_pos) {
        parse_start_str_pos = new int;
        *parse_start_str_pos = 0;
        isroot = true;
    }
    
    int a = *parse_start_str_pos;
    
    while (*parse_start_str_pos < str_p->length()) { // *
        
        // initial whitespace characters
        if (initial_whitespaces) {
            if (str[a] == ' ' || str[a] == '\n' || str[a] == '\r' || str[a] == '\t' ) {
                ++a;
                continue;
            }
            else {
                if (str[a] == '{') // start of object
                    // ... TODO: seek_next ':'
                
                initial_whitespaces = false;
            }
        }
        
        
        // delimiter
        if ( bracket_stack.size()==0  &&  quote_stack.size()==0  &&  str[a]==json_arraydelimiter ) {
            //ret.push_back (current);
            
            //current.clear();
            bracket_stack.clear(); quote_stack.clear(); escape_active = false;
            continue; // to *
        }
        
        // ------------------------------------
        // checks for string
        
        if (quote_stack.size() > 0) { // already inside string
            if (str[a]==json_charescape)  // an escape character
                escape_active = !escape_active;
            else if (!escape_active  &&  str[a]==json_stringquotes[quote_stack.back()][1] ) { // close quote
                quote_stack.pop_back();
                escape_active = false;
            }
            else
                escape_active = false;
            
            //current.push_back (str[a]);
            continue; // to *
        }
        
        if (quote_stack.size()==0) { // check for start of string
            if ((bi = is_bracket (str[a], json_stringquotes)) >= 0) {
                quote_stack.push_back (bi);
                //current.push_back (str[a]);
                continue; // to *
            }
        }
        
        // ------------------------------------
        // checks for comments
        
        if (quote_stack.size()==0) { // comment cannot start inside string
            
            // single-line commenst
            if (str.compare (a, json_linecommentstart.length(), json_linecommentstart) == 0) {
                // ignore until end of line
                int newline_pos = str.find ("\n", a);
                if (newline_pos == std::string::npos)
                    newline_pos = str.find ("\r", a);
                
                if (newline_pos != std::string::npos)
                    a = newline_pos; // point to the newline character (a will be incremented)
                else // the comment continues until EOF
                    a = str.length();
                continue;
            }
        }
        
        // ------------------------------------
        // checks for brackets
        
        if ( bracket_stack.size()>0  &&  str[a]==json_brackets[bracket_stack.back()][1] ) { // check for closing bracket
            bracket_stack.pop_back();
            //current.push_back (str[a]);
            continue;
        }
        
        if ((bi = is_bracket (str[a], json_brackets)) >= 0) {
            bracket_stack.push_back (bi);
            //current.push_back (str[a]);
            continue; // to *
        }
        
        // ------------------------------------
        // otherwise
        //current.push_back (str[a]);
    }
    
    /*if (current.length() > 0)
        ret.push_back (current); */
    
    if (isroot)
        delete parse_start_str_pos;
    
    // return (ret);
}

// ============================================================

// ------------------------------------------------------------

inline 
json_object& json_resource::as_object (bool force) {
    if (!parsed_data_p)  parsed_data_p = new json_parsedData;
    if (parsed_data_p->type==JSON_UNKNOWN || force)  parsed_data_p->parse (data, JSON_OBJECT);
    return (parsed_data_p->object);
}

inline 
json_resource& json_resource::operator[] (const char *key) { // returns reference
    return ( (as_object())[std::string(key)] ); // will return empty resource (with _exists==false) if 
                                            // either this resource does not exist, is not an object, or the key does not exist
}
inline 
json_resource& json_resource::operator[] (std::string key) { // returns reference
    return ( (as_object())[key] ); // will return empty resource (with _exists==false) if 
                                            // either this resource does not exist, is not an object, or the key does not exist
}

inline 
json_array& json_resource::as_array (bool force) {
    if (!parsed_data_p)  parsed_data_p = new json_parsedData;
    if (parsed_data_p->type==JSON_UNKNOWN || force)  parsed_data_p->parse (data, JSON_ARRAY);
    return (parsed_data_p->array);
}

inline 
json_resource& json_resource::operator[] (int indx) { // returns reference
    as_array();
    if (indx >= parsed_data_p->array.size())
        parsed_data_p->array.resize(indx+1); // insert empty resources
    return (parsed_data_p->array[indx]); // will return empty resource (with _exists==false) if 
                                            // either this resource does not exist, is not an object, or the key does not exist
}

// ------------------------------------------------------------
// special 'as':

template <class dataType, class vectorType> inline 
vectorType json_resource::as_vector (const vectorType& def) { // returns copy -- for being consistent with other 'as' specializations
    if (!exists()) return (def);
    vectorType ret;
    as_array();
    for (auto it=parsed_data_p->array.begin(); it!=parsed_data_p->array.end(); ++it)
        ret.push_back (it->as<dataType>());
    return (ret);
}

template <class dataType, class mapType> inline 
mapType json_resource::as_map (const mapType& def) { // returns copy -- for being consistent with other 'as' specializations
    if (!exists()) return (def);
    mapType ret;
    as_object();
    for (auto it=parsed_data_p->object.begin(); it!=parsed_data_p->object.end(); ++it)
        ret[it->first] = it->second.as<dataType>();
    return (ret);
}

// ============================================================
// Specialized .as() member functions

// Helper preprocessor directives
#define rsjObject  as<json_object>()
#define rsjArray   as<json_array>()
#define rsjAs(t)   as<t>()


// json_object
template <> inline 
json_object json_resource::as<json_object> (const json_object& def) { // returns copy -- for being consistent with other 'as' specializations
    if (!exists()) return (def);
    return (as_object());
}

// json_array
template <> inline 
json_array  json_resource::as<json_array> (const json_array& def) { // returns copy -- for being consistent with other 'as' specializations
    if (!exists()) return (def);
    return (as_array());
}

// ------------------------------------
// Elementary types

// String
template <> inline 
std::string  json_resource::as<std::string> (const std::string& def) {
    if (!exists()) return (def);
    
    char qq = '\0';
    std::string ret = strip_outer_quotes (data, &qq);
    
    std::vector< std::vector<std::string> > escapes = { {"\\n","\n"}, {"\\r","\r"}, {"\\t","\t"}, {"\\\\","\\"} };
    if (qq=='"')
        escapes.push_back ({"\\\"","\""});
    else if (qq=='\'')
        escapes.push_back ({"\\'","'"});
    
    for (int a=0; a<escapes.size(); ++a)
        for ( std::size_t start_pos=ret.find(escapes[a][0]); start_pos!=std::string::npos; start_pos=ret.find(escapes[a][0],start_pos) ) {
            ret.replace (start_pos, escapes[a][0].length(), escapes[a][1]);
            start_pos += escapes[a][1].length();
        }
    
    return (ret);
}

// integer
template <> inline 
int  json_resource::as<int> (const int& def) {
    if (!exists()) return (def);
    return (atoi (strip_outer_quotes(data).c_str() ) );
}

// double
template <> inline 
double  json_resource::as<double> (const double& def) {
    if (!exists()) return (def);
    return (atof (strip_outer_quotes(data).c_str() ) );
}

// bool
template <> inline 
bool  json_resource::as<bool> (const bool& def) {
    if (!exists()) return (def);
    std::string cleanData = strip_outer_quotes (data);
    if (cleanData=="true" || cleanData=="TRUE" || cleanData=="True" || atoi(cleanData.c_str())!=0) return (true);
    return (false);
}
