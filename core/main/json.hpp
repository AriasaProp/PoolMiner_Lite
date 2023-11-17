#ifndef JSON_
#define JSON_

#include <cctype>
#include <cmath>
#include <cstdint>
#include <deque>
#include <initializer_list>
#include <iostream>
#include <map>
#include <ostream>
#include <string>
#include <type_traits>

namespace json {

namespace {
  std::string json_escape (const std::string &str) {
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
} // namespace

class JSON {
  union BackingData {
    BackingData (double d) : Float (d) {}
    BackingData (long l) : Int (l) {}
    BackingData (bool b) : Bool (b) {}
    BackingData (std::string s) : String (new std::string (s)) {}
    BackingData () : Int (0) {}

    std::deque<JSON> *List;
    std::map<std::string, JSON> *Map;
    std::string *String;
    double Float;
    long Int;
    bool Bool;
  } Internal;

public:
  enum class Class {
    Null,
    Object,
    Array,
    String,
    Floating,
    Integral,
    Boolean
  };

  template <typename Container>
  class JSONWrapper {
    Container *object;

  public:
    JSONWrapper (Container *val) : object (val) {}
    JSONWrapper (std::nullptr_t) : object (nullptr) {}

    typename Container::iterator begin () { return object ? object->begin () : typename Container::iterator (); }
    typename Container::iterator end () { return object ? object->end () : typename Container::iterator (); }
    typename Container::const_iterator begin () const { return object ? object->begin () : typename Container::iterator (); }
    typename Container::const_iterator end () const { return object ? object->end () : typename Container::iterator (); }
  };

  template <typename Container>
  class JSONConstWrapper {
    const Container *object;

  public:
    JSONConstWrapper (const Container *val) : object (val) {}
    JSONConstWrapper (std::nullptr_t) : object (nullptr) {}

    typename Container::const_iterator begin () const { return object ? object->begin () : typename Container::const_iterator (); }
    typename Container::const_iterator end () const { return object ? object->end () : typename Container::const_iterator (); }
  };

  JSON () : Internal (), Type (Class::Null) {}

  JSON (std::initializer_list<JSON> list)
      : JSON () {
    SetType (Class::Object);
    for (auto i = list.begin (), e = list.end (); i != e; ++i, ++i)
      operator[] ((std::string)*i) = *std::next (i);
  }

  JSON (JSON &&other)
      : Internal (other.Internal), Type (other.Type) {
    other.Type = Class::Null;
    other.Internal.Map = nullptr;
  }

  JSON &operator= (JSON &&other) {
    ClearInternal ();
    Internal = other.Internal;
    Type = other.Type;
    other.Internal.Map = nullptr;
    other.Type = Class::Null;
    return *this;
  }

  JSON (const JSON &other) {
    switch (other.Type) {
    case Class::Object:
      Internal.Map =
          new std::map<std::string, JSON> (other.Internal.Map->begin (),
                                           other.Internal.Map->end ());
      break;
    case Class::Array:
      Internal.List =
          new std::deque<JSON> (other.Internal.List->begin (),
                                other.Internal.List->end ());
      break;
    case Class::String:
      Internal.String =
          new std::string (*other.Internal.String);
      break;
    default:
      Internal = other.Internal;
    }
    Type = other.Type;
  }

  JSON &operator= (const JSON &other) {
    ClearInternal ();
    switch (other.Type) {
    case Class::Object:
      Internal.Map =
          new std::map<std::string, JSON> (other.Internal.Map->begin (),
                                           other.Internal.Map->end ());
      break;
    case Class::Array:
      Internal.List =
          new std::deque<JSON> (other.Internal.List->begin (),
                                other.Internal.List->end ());
      break;
    case Class::String:
      Internal.String =
          new std::string (*other.Internal.String);
      break;
    default:
      Internal = other.Internal;
    }
    Type = other.Type;
    return *this;
  }

  ~JSON () {
    switch (Type) {
    case Class::Array:
      delete Internal.List;
      break;
    case Class::Object:
      delete Internal.Map;
      break;
    case Class::String:
      delete Internal.String;
      break;
    default:;
    }
  }

  template <typename T>
  JSON (T b, typename std::enable_if<std::is_same<T, bool>::value>::type * = 0) : Internal (b), Type (Class::Boolean) {}

  template <typename T>
  JSON (T i, typename std::enable_if<std::is_integral<T>::value && !std::is_same<T, bool>::value>::type * = 0) : Internal ((long)i), Type (Class::Integral) {}

  template <typename T>
  JSON (T f, typename std::enable_if<std::is_floating_point<T>::value>::type * = 0) : Internal ((double)f), Type (Class::Floating) {}

  template <typename T>
  JSON (T s, typename std::enable_if<std::is_convertible<T, std::string>::value>::type * = 0) : Internal (std::string (s)), Type (Class::String) {}

  JSON (std::nullptr_t) : Internal (), Type (Class::Null) {}

  static JSON Make (Class type) {
    JSON ret;
    ret.SetType (type);
    return ret;
  }

  static JSON Load (const std::string &);

  template <typename T>
  void append (T arg) {
    SetType (Class::Array);
    Internal.List->emplace_back (arg);
  }

  template <typename T, typename... U>
  void append (T arg, U... args) {
    append (arg);
    append (args...);
  }

  template <typename T>
  typename std::enable_if<std::is_same<T, bool>::value, JSON &>::type operator= (T b) {
    SetType (Class::Boolean);
    Internal.Bool = b;
    return *this;
  }

  template <typename T>
  typename std::enable_if<std::is_integral<T>::value && !std::is_same<T, bool>::value, JSON &>::type operator= (T i) {
    SetType (Class::Integral);
    Internal.Int = i;
    return *this;
  }

  template <typename T>
  typename std::enable_if<std::is_floating_point<T>::value, JSON &>::type operator= (T f) {
    SetType (Class::Floating);
    Internal.Float = f;
    return *this;
  }

  template <typename T>
  typename std::enable_if<std::is_convertible<T, std::string>::value, JSON &>::type operator= (T s) {
    SetType (Class::String);
    *Internal.String = std::string (s);
    return *this;
  }

  JSON &operator[] (const char *key) {
    SetType (Class::Object);
    return Internal.Map->operator[] (std::string (key));
  }
  JSON &operator[] (const std::string &key) {
    SetType (Class::Object);
    return Internal.Map->operator[] (key);
  }

  JSON &operator[] (unsigned index) {
    SetType (Class::Array);
    if (index >= Internal.List->size ()) Internal.List->resize (index + 1);
    return Internal.List->operator[] (index);
  }

  int length () const {
    if (Type == Class::Array)
      return Internal.List->size ();
    else
      return -1;
  }

  bool hasKey (const std::string &key) const {
    if (Type == Class::Object)
      return Internal.Map->find (key) != Internal.Map->end ();
    return false;
  }

  int size () const {
    if (Type == Class::Object)
      return Internal.Map->size ();
    else if (Type == Class::Array)
      return Internal.List->size ();
    else
      return -1;
  }

  Class JSONType () const { return Type; }

  /// Functions for getting primitives from the JSON object.
  bool IsNull () const { return Type == Class::Null; }

  operator std::string () const {
    return (Type == Class::String) ? std::move (json_escape (*Internal.String)) : std::string ("");
  }
  operator double () const {
    return (Type == Class::Floating) ? Internal.Float : 0.0;
  }
  operator float () const {
    return (Type == Class::Floating) ? (float)Internal.Float : 0.0f;
  }
  operator long () const {
    return (Type == Class::Integral) ? Internal.Int : 0;
  }
  operator int () const {
    return (Type == Class::Integral) ? (int)Internal.Int : 0.0f;
  }
  operator bool () const {
    return (Type == Class::Boolean) ? Internal.Bool : false;
  }

  JSONWrapper<std::map<std::string, JSON>> ObjectRange () {
    if (Type == Class::Object)
      return JSONWrapper<std::map<std::string, JSON>> (Internal.Map);
    return JSONWrapper<std::map<std::string, JSON>> (nullptr);
  }

  JSONWrapper<std::deque<JSON>> ArrayRange () {
    if (Type == Class::Array)
      return JSONWrapper<std::deque<JSON>> (Internal.List);
    return JSONWrapper<std::deque<JSON>> (nullptr);
  }

  JSONConstWrapper<std::map<std::string, JSON>> ObjectRange () const {
    if (Type == Class::Object)
      return JSONConstWrapper<std::map<std::string, JSON>> (Internal.Map);
    return JSONConstWrapper<std::map<std::string, JSON>> (nullptr);
  }

  JSONConstWrapper<std::deque<JSON>> ArrayRange () const {
    if (Type == Class::Array)
      return JSONConstWrapper<std::deque<JSON>> (Internal.List);
    return JSONConstWrapper<std::deque<JSON>> (nullptr);
  }

  std::string dump (int depth = 1, std::string tab = "  ") const {
    std::string pad = "";
    for (int i = 0; i < depth; ++i, pad += tab)
      ;

    switch (Type) {
    case Class::Null:
      return "null";
    case Class::Object: {
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
    case Class::Array: {
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
    case Class::String:
      return "\"" + json_escape (*Internal.String) + "\"";
    case Class::Floating:
      return std::to_string (Internal.Float);
    case Class::Integral:
      return std::to_string (Internal.Int);
    case Class::Boolean:
      return Internal.Bool ? "true" : "false";
    default:
      return "";
    }
    return "";
  }

  friend std::ostream &operator<< (std::ostream &, const JSON &);

private:
  void SetType (Class type) {
    if (type == Type)
      return;

    ClearInternal ();

    switch (type) {
    case Class::Null:
      Internal.Map = nullptr;
      break;
    case Class::Object:
      Internal.Map = new std::map<std::string, JSON> ();
      break;
    case Class::Array:
      Internal.List = new std::deque<JSON> ();
      break;
    case Class::String:
      Internal.String = new std::string ();
      break;
    case Class::Floating:
      Internal.Float = 0.0;
      break;
    case Class::Integral:
      Internal.Int = 0;
      break;
    case Class::Boolean:
      Internal.Bool = false;
      break;
    }

    Type = type;
  }

private:
  /* beware: only call if YOU know that Internal is allocated. No checks performed here.
     This function should be called in a constructed JSON just before you are going to
    overwrite Internal...
  */
  void ClearInternal () {
    switch (Type) {
    case Class::Object:
      delete Internal.Map;
      break;
    case Class::Array:
      delete Internal.List;
      break;
    case Class::String:
      delete Internal.String;
      break;
    default:;
    }
  }

private:
  Class Type = Class::Null;
};

JSON Array ();

template <typename... T>
JSON Array (T... args);

JSON Object ();
std::ostream &operator<< (std::ostream &, const json::JSON &);
} // End Namespace json

#endif // JSON_