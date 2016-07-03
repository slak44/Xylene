#ifndef OBJECT_HPP
#define OBJECT_HPP

#include <string>
#include <set>
#include <numeric>

#include "util.hpp"
#include "error.hpp"

typedef std::set<std::string> TypeList;

class Object {
public:
  typedef PtrUtil<Object>::Link Link;
  typedef PtrUtil<Object>::WeakLink WeakLink;
  virtual ~Object() {}
  
  virtual bool isTruthy() const = 0;
  virtual std::string getTypeName() const = 0;
  virtual std::string toString() const = 0;
};

class Reference: public Object {
private:
  Object::Link ref;
  bool isDynamic;
  TypeList allowed;
  
  void throwIfNull() const {
    if (!ref) throw InternalError("Null reference access", {METADATA_PAIRS});
  }
public:
  typedef PtrUtil<Reference>::Link Link;
  typedef PtrUtil<Reference>::WeakLink WeakLink;
  
  Reference(Object::Link obj): ref(obj), isDynamic(true), allowed({}) {}
  Reference(Object::Link obj, TypeList list): ref(obj), isDynamic(false), allowed(list) {}
  
  Object::Link getValue() const {
    throwIfNull();
    return ref;
  }
  
  void setValue(Object::Link newRef) {
    if (!canStoreType(newRef->getTypeName())) throw InternalError("Type mismatch", {
      METADATA_PAIRS,
      {"new type", newRef->getTypeName()},
      {"type list", std::accumulate(ALL(allowed), std::string {},
        [](std::string prev, std::string curr) {return prev + " " + curr;})},
      {"is dynamic", std::to_string(isDynamic)}
    });
    ref = newRef;
  }
  
  bool canStoreType(std::string typeName) const {
    if (isDynamic) return true;
    else return std::find(ALL(allowed), typeName) != allowed.end();
  }
  
  bool isTruthy() const {
    throwIfNull();
    return ref->isTruthy();
  }
  
  std::string getTypeName() const {
    return "Reference";
  }
  
  std::string toString() const {
    throwIfNull();
    return "Reference " + getAddressStringFrom(this) + ": " + ref->toString();
  }
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
  
  std::string toString() const {
    return "Integer " + std::to_string(internal) + " (" + getAddressStringFrom(this) + ")";
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
  
  std::string toString() const {
    return "Float " + std::to_string(internal) + " (" + getAddressStringFrom(this) + ")";
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
  
  std::string toString() const {
    return "String \'" + internal + "\' (" + getAddressStringFrom(this) + ")";
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
  
  std::string toString() const {
    return "Boolean " + std::to_string(internal) + " (" + getAddressStringFrom(this) + ")";
  }
  
  std::string getTypeName() const {
    return "Boolean";
  }
};

#define OSTREAM_OVERLOAD(type) \
std::ostream& operator<<(std::ostream& os, type obj) {\
  return os << obj.toString();\
}

OSTREAM_OVERLOAD(Integer);
OSTREAM_OVERLOAD(Float);
OSTREAM_OVERLOAD(String);
OSTREAM_OVERLOAD(Boolean);

#undef OSTREAM_OVERLOAD

#endif
