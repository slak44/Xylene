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
  class Boolean : public Object {
  private:
    bool internal = false;
  public:
    Boolean() {}
    Boolean(bool b): internal(b) {}
    Boolean(std::string str): internal(str == "true" ? true : false) {}
    
    bool isTruthy() {return this->internal;}
    std::string toString() {return this->internal ? "true" : "false";}
    std::string getTypeData() {return "Boolean";}
    
    bool value() {return this->internal;}
  };
  
  class String : public Object {
  private:
    std::string internal = "";
  public:
    String() {}
    String(std::string str): internal(str) {}
    
    bool isTruthy() {return this->internal.size() > 0;}
    std::string toString() {return this->internal;}
    std::string getString() {return this->toString();}
    std::string getTypeData() {return "String";}
  };
  
  class Float : public Object {
  private:
    double64 internal = 0.0;
  public:
    Float() {}
    Float(double64 f): internal(f) {}
    Float(std::string str): internal(std::stold(str)) {}
    
    bool isTruthy() {return this->internal != 0.0;}
    std::string toString() {return std::to_string(internal);}
    std::string getTypeData() {return "Float";}
    
    double64 getNumber() {return this->internal;}
  };
  
  class Integer : public Object {
  private:
    int64 internal = 0;
  public:
    Integer() {}
    Integer(int64 i): internal(i) {}
    Integer(std::string str, int base): internal(std::stoll(str, 0, base)) {}
    
    bool isTruthy() {return this->internal != 0;}
    std::string toString() {return std::to_string(internal);}
    std::string getTypeData() {return "Integer";}
    
    int64 getNumber() {return this->internal;}
  };
  
}; /* namespace lang */

#endif /* BUILTINS_H_ */
