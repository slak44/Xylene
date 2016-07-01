#ifndef OBJECT_HPP
#define OBJECT_HPP

#include <string>

#include "util.hpp"

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
  
  int64 getValue() const {
    return internal;
  }
  
  bool isTruthy() const {
    return internal == 0;
  }
  
  std::string getTypeName() const {
    return "Integer";
  }
};

class Float: public Object {
private:
  long double internal = 0;
public:
  Float() {}
  Float(long double ld): internal(ld) {}
  Float(std::string str): internal(std::stold(str, 0)) {}
  
  long double getValue() const {
    return internal;
  }
  
  bool isTruthy() const {
    return internal == 0;
  }
  
  std::string getTypeName() const {
    return "Float";
  }
};

class String: public Object {
private:
  std::string internal = "";
public:
  String() {}
  String(std::string str): internal(str) {}
  
  std::string getValue() const {
    return internal;
  }
  
  bool isTruthy() const {
    return internal != "";
  }
  
  std::string getTypeName() const {
    return "String";
  }
};

class Boolean: public Object {
private:
  bool internal = false;
public:
  Boolean() {}
  Boolean(bool b): internal(b) {}
  Boolean(std::string b): internal(b == "true") {}
  
  bool getValue() const {
    return internal;
  }
  
  bool isTruthy() const {
    return internal;
  }
  
  std::string getTypeName() const {
    return "Boolean";
  }
};

#define OSTREAM_OVERLOAD(type) \
std::ostream& operator<<(std::ostream& os, type obj) {\
  return os << #type << " " << obj.getValue() << " (" << &obj << ")";\
}

OSTREAM_OVERLOAD(Integer);
OSTREAM_OVERLOAD(Float);
OSTREAM_OVERLOAD(String);
OSTREAM_OVERLOAD(Boolean);

#undef OSTREAM_OVERLOAD

#endif
