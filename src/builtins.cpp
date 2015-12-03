#include "builtins.hpp"

namespace lang {
  
  void concatenateNames(std::string& result, unsigned int line) {
    result.pop_back(); // Remove trailing space
  }
  
  Object::Object() {}
  Object::~Object() {}
  
  std::string Object::toString() {
    return std::to_string((int64) this);
  }
  
  std::string Object::getTypeData() {return "Object";}
  
  Variable::Variable() {}
  Variable::Variable(Object* obj): internal(obj) {}
  
  bool Variable::isTruthy() {return this->read()->isTruthy();}
  std::string Variable::toString() {
    std::stringstream ss; ss << this;
    return std::string("Variable ") + ss.str() + ":" + this->internal->toString();
  }
  std::string Variable::getTypeData() {return "Variable";}
  
  void Variable::assign(Object* newObj) {
    this->internal = newObj;
  }
  Object* Variable::read() {
    return this->internal;
  }
  
  String::String() {}
  String::String(std::string str): internal(str) {}
  
  bool String::isTruthy() {return this->internal.size() > 0;}
  std::string String::toString() {return this->internal;}
  std::string String::getString() {return this->toString();}  
  std::string String::getTypeData() {return "String";}
  
  Boolean::Boolean() {}
  Boolean::Boolean(bool b): internal(b) {}
  Boolean::Boolean(std::string str): internal(str == "true" ? true : false) {}
  
  bool Boolean::isTruthy() {return this->internal;}
  std::string Boolean::toString() {return this->internal ? "true" : "false";}
  std::string Boolean::getTypeData() {return "Boolean";}
  
  bool Boolean::value() {return this->internal;}
  
  Float::Float() {}
  Float::Float(double64 f): internal(f) {}
  Float::Float(std::string str) {
    this->internal = std::stold(str);
  }
  
  bool Float::isTruthy() {return this->internal != 0;}
  std::string Float::toString() {return std::to_string(internal);}
  std::string Float::getTypeData() {return "Float";}
  
  double64 Float::getNumber() {return this->internal;}
  
  Integer::Integer() {}
  Integer::Integer(int64 i): internal(i) {}
  Integer::Integer(std::string str, int base) {
    this->internal = std::stoll(str, 0, base);
  }
  
  bool Integer::isTruthy() {return this->internal != 0;}
  std::string Integer::toString() {return std::to_string(internal);}
  std::string Integer::getTypeData() {return "Integer";}
  
  int64 Integer::getNumber() {return this->internal;}
  
} /* namespace lang */
