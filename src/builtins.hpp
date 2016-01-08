#ifndef BUILTINS_H_
#define BUILTINS_H_

#include <string>
#include <sstream>
#include <unordered_map>
#include <functional>
#include <stdexcept>
#include <climits>
#include <cfloat>
#include <boost/any.hpp>

#include "global.hpp"
#include "objects.hpp"

namespace lang {
  class Boolean : public Instance {
  private:
    bool internal = false;
  public:
    static Type* booleanType;
    Boolean():
      Instance(booleanType) {}
    Boolean(bool b):
      Instance(booleanType),
      internal(b) {}
    Boolean(std::string str):
      Instance(booleanType),
      internal(str == "true" ? true : false) {}
    
    bool isTruthy() {return this->internal;}
    std::string toString() {return this->internal ? "true" : "false";}
    std::string getTypeData() {return "Boolean";}
    
    bool value() {return this->internal;}
  };
  
  class String : public Instance {
  private:
    std::string internal = "";
  public:
    static Type* stringType;
    String():
      Instance(stringType) {}
    String(std::string str):
      Instance(stringType),
      internal(str) {}
    
    bool isTruthy() {return this->internal.size() > 0;}
    std::string toString() {return this->internal;}
    std::string getString() {return this->toString();}
    std::string getTypeData() {return "String";}
  };
  
  class Float : public Instance {
  private:
    double64 internal = 0.0;
  public:
    static Type* floatType;
    Float():
      Instance(floatType) {}
    Float(double64 f):
      Instance(floatType),
      internal(f) {}
    Float(std::string str):
      Instance(floatType),
      internal(std::stold(str)) {}
    
    bool isTruthy() {return this->internal != 0.0;}
    std::string toString() {return std::to_string(internal);}
    std::string getTypeData() {return "Float";}
    
    double64 getNumber() {return this->internal;}
  };
  
  class Integer : public Instance {
  private:
    int64 internal = 0;
  public:
    static Type* integerType;
    Integer():
      Instance(integerType) {}
    Integer(int64 i):
      Instance(integerType),
      internal(i) {}
    Integer(std::string str, int base):
      Instance(integerType),
      internal(std::stoll(str, 0, base)) {}
    
    bool isTruthy() {return this->internal != 0;}
    std::string toString() {return std::to_string(internal);}
    std::string getTypeData() {return "Integer";}
    
    int64 getNumber() {return this->internal;}
  };
  
}; /* namespace lang */

#endif /* BUILTINS_H_ */
