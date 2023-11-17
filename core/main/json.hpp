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

struct JSON {
  Class Type = Class::Null;
  void SetType (Class type) {
    if (type == Type) return;
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

  template <typename Container>
  struct JSONWrapper {
    private:
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
  struct JSONConstWrapper {
    private:
      const Container *object;

    public:
      JSONConstWrapper (const Container *val) : object (val) {}
      JSONConstWrapper (std::nullptr_t) : object (nullptr) {}
  
      typename Container::const_iterator begin () const { return object ? object->begin () : typename Container::const_iterator (); }
      typename Container::const_iterator end () const { return object ? object->end () : typename Container::const_iterator (); }
  };
  enum class Class {
    Null,
    Object,
    Array,
    String,
    Floating,
    Integral,
    Boolean
  };
  
  JSON ();
  JSON (std::initializer_list<JSON>);
  JSON (JSON &&);
  JSON (const JSON &);

  JSON &operator=(JSON &&);
  JSON &operator= (const JSON &);

  ~JSON ();

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
  
  operator std::string() const;
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

  std::string dump (int, std::string) const;
  friend std::ostream &operator<< (std::ostream &, const JSON &);

};

JSON Array ();

template <typename... T>
JSON Array (T... args);

JSON Object ();
std::ostream &operator<< (std::ostream &, const json::JSON &);
} // End Namespace json

#endif // JSON_