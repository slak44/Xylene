#include "builtins.hpp"

namespace lang {
  
  void concatenateNames(std::string& result) {
    result.pop_back(); // Remove trailing space
  }
  
  Object::Object() {
  }
  
  Object::~Object() {
    
  }
  
  std::string Object::asString() {
    return std::to_string((int64) this);
  }
  
  std::string Object::getTypeData() {
    return "Object";
  }
  
  Float::Float() {}
  
  Float::Float(double64 f): internal(f) {}
  
  Float::Float(std::string str) {
    this->internal = std::stold(str);
  }
  
  std::string Float::asString() {
    return std::to_string(internal);
  }
  
  std::string Float::getTypeData() {
    return "Float";
  }
  
  double64 Float::getNumber() {
    return this->internal;
  }
  
  Integer::Integer() {}
  
  Integer::Integer(int64 i): internal(i) {}
  
  Integer::Integer(std::string str, int base) {
    this->internal = std::stoll(str, 0, base);
  }
  
  std::string Integer::asString() {
    return std::to_string(internal);
  }
  
  std::string Integer::getTypeData() {
    return "Integer";
  }
  
  int64 Integer::getNumber() {
    return this->internal;
  }
  
} /* namespace lang */
