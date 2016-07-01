#ifndef OBJECT_HPP
#define OBJECT_HPP

#include <string>

#include "util.hpp"

#define OSTREAM_OVERLOAD(type, fetchData) \
std::ostream& operator<<(std::ostream& os, type obj) {\
  return os << #type << " " << obj. fetchData << " (" << &obj << ")";\
}

class Object {
public:
  typedef PtrUtil<Object>::Link Link;
  virtual ~Object() {}
  
  virtual bool isTruthy() const = 0;
  virtual std::string getTypeName() const = 0;
};

class Integer: public Object {
private:
  int64 internal = 0;
public:
  Integer() {}
  Integer(int64 i): internal(i) {}
  Integer(std::string str, int base): internal(std::stoll(str, 0, base)) {}
  
  int64 getInt() const {
    return internal;
  }
  
  bool isTruthy() const {
    return internal == 0;
  }
  
  std::string getTypeName() const {
    return "Integer";
  }
};

OSTREAM_OVERLOAD(Integer, getInt());

class Float: public Object {
private:
  long double internal = 0;
public:
  Float() {}
  Float(long double ld): internal(ld) {}
  Float(std::string str): internal(std::stold(str, 0)) {}
  
  long double getFloat() const {
    return internal;
  }
  
  bool isTruthy() const {
    return internal == 0;
  }
  
  std::string getTypeName() const {
    return "Float";
  }
};

OSTREAM_OVERLOAD(Float, getFloat());

class String: public Object {
private:
  std::string internal = "";
public:
  String() {}
  String(std::string str): internal(str) {}
  
  std::string getString() const {
    return internal;
  }
  
  bool isTruthy() const {
    return internal != "";
  }
  
  std::string getTypeName() const {
    return "String";
  }
};

OSTREAM_OVERLOAD(String, getString());

class Boolean: public Object {
private:
  bool internal = false;
public:
  Boolean() {}
  Boolean(bool b): internal(b) {}
  Boolean(std::string b): internal(b == "true") {}
  
  bool getBoolean() const {
    return internal;
  }
  
  bool isTruthy() const {
    return internal;
  }
  
  std::string getTypeName() const {
    return "Boolean";
  }
};

OSTREAM_OVERLOAD(Boolean, getBoolean());


#endif
