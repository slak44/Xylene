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

std::ostream& operator<<(std::ostream& os, Integer in) {
  return os << "Integer " << in.getInt() << " (" << &in << ")";
}

#endif
