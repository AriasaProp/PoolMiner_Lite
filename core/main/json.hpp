#ifndef JSON_
#define JSON_

#include <cctype>
#include <cmath>
#include <cstdint>
#include <deque>
#include <iostream>
#include <ostream>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>

namespace json {

struct JSON {
  enum class Class {
    Null,
    Object,
    Array,
    String,
    Floating,
    Integral,
    Boolean
  };
  union BackingData {
    BackingData (double d) : Float (d) {}
    BackingData (long l) : Int (l) {}
    BackingData (bool b) : Bool (b) {}
    BackingData (std::string s) : String (new std::string (s)) {}
    BackingData () : Int (0) {}

    std::deque<JSON> *List;
    std::unordered_map<std::string, JSON> *Map;
    std::string *String;
    double Float;
    long Int;
    bool Bool;
  } Internal;
  Class Type = Class::Null;

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

  JSON ();
  JSON (Class);
  JSON (JSON &&other);
  JSON (const JSON &other);

  JSON &operator= (JSON &&);
  JSON &operator= (const JSON &);

  template <typename T>
  JSON (T b, typename std::enable_if<std::is_same<T, bool>::value>::type * = 0) : Internal (b), Type (Class::Boolean) {}

  template <typename T>
  JSON (T i, typename std::enable_if<std::is_integral<T>::value && !std::is_same<T, bool>::value>::type * = 0) : Internal ((long)i), Type (Class::Integral) {}

  template <typename T>
  JSON (T f, typename std::enable_if<std::is_floating_point<T>::value>::type * = 0) : Internal ((double)f), Type (Class::Floating) {}

  template <typename T>
  JSON (T s, typename std::enable_if<std::is_convertible<T, std::string>::value>::type * = 0) : Internal (std::string (s)), Type (Class::String) {}

  JSON (std::nullptr_t) : Internal (), Type (Class::Null) {}
  ~JSON ();

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
  JSON &operator[] (const char *);
  JSON &operator[] (const std::string &);
  JSON &operator[] (size_t);
  int length () const;
  bool hasKey (const std::string &) const;
  int size () const;
  Class JSONType () const;
  bool IsNull () const;
  operator std::string () const;
  explicit operator double () const;
  explicit operator float () const;
  explicit operator long () const;
  explicit operator int () const;
  explicit operator bool () const;

  JSONWrapper<std::unordered_map<std::string, JSON>> ObjectRange () {
    if (Type == Class::Object)
      return JSONWrapper<std::unordered_map<std::string, JSON>> (Internal.Map);
    return JSONWrapper<std::unordered_map<std::string, JSON>> (nullptr);
  }

  JSONWrapper<std::deque<JSON>> ArrayRange () {
    if (Type == Class::Array)
      return JSONWrapper<std::deque<JSON>> (Internal.List);
    return JSONWrapper<std::deque<JSON>> (nullptr);
  }

  JSONConstWrapper<std::unordered_map<std::string, JSON>> ObjectRange () const {
    if (Type == Class::Object)
      return JSONConstWrapper<std::unordered_map<std::string, JSON>> (Internal.Map);
    return JSONConstWrapper<std::unordered_map<std::string, JSON>> (nullptr);
  }

  JSONConstWrapper<std::deque<JSON>> ArrayRange () const {
    if (Type == Class::Array)
      return JSONConstWrapper<std::deque<JSON>> (Internal.List);
    return JSONConstWrapper<std::deque<JSON>> (nullptr);
  }

  std::string dump (int, std::string) const;
  void SetType (Class);
  void ClearInternal ();

  friend std::ostream &operator<< (std::ostream &, const JSON &);
};

JSON parse (const std::string &);

JSON Array ();

template <typename... T>
JSON Array (T... args);

JSON Object ();
std::ostream &operator<< (std::ostream &, const JSON &);
} // End Namespace json

#endif // JSON_