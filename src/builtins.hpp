#ifndef BUILTINS_H_
#define BUILTINS_H_

#include <string>
#include <unordered_map>
#include <functional>
#include <stdexcept>
#include <cmath>
#include <boost/any.hpp>

#include "global.hpp"
#include "operators.hpp"

namespace lang {

  typedef std::unordered_map<Operator, std::unordered_map<std::string, boost::any>, OperatorHash> OperatorMap;
  extern OperatorMap opsMap;
  
  class Object {
  public:
    Object();
    virtual ~Object();
    
    virtual std::string asString();
    virtual std::string getTypeData() = 0; // Pure virtual, derived classes must implement this
  };
  
  inline std::ostream& operator<<(std::ostream& os, Object& obj) { 
    return os << "Object[" << obj.asString() << "]";
  }
  
  void concatenateNames(std::string& result);
  
  template<typename... Args>
  void concatenateNames(std::string& result, Object* obj, Args... args) {
    result += obj->getTypeData() + " ";
    concatenateNames(result, args...);
  }
  
  template<typename... Args>
  Object* runOperator(Operator* op, Args... pr) {
    std::string funSig = "";
    concatenateNames(funSig, pr...);
    // TODO: handle case where operator function is undefined for given types
    // 1. Get a boost::any instance from the OperatorMap
    // 2. Use boost::any_cast to get a pointer to the operator function
    // 3. Dereference pointer and call function
    auto result = (*boost::any_cast<std::function<Object*(Args...)>*>(opsMap[*op][funSig]))(pr...);
    return result;
  }
  
  class String : public Object {
  private:
    std::string internal = "";
  public:
    String();
    String(std::string str);
    
    std::string asString();
    std::string getTypeData();
    
    std::string getString();
};
  
  // TODO: Number<T> class, with T = int64 || double64
  
  class Float : public Object {
  private:
    double64 internal = 0.0;
  public:
    typedef std::function<Object*(Float*, Float*)> BinaryOp;
    typedef std::function<Object*(Float*)> UnaryOp;
    
    Float();
    Float(std::string str);
    Float(double64 f);
    
    std::string asString();
    std::string getTypeData();
    
    double64 getNumber();
  };
  
  class Integer : public Object {
  private:
    int64 internal = 0;
  public:
    typedef std::function<Object*(Integer*, Integer*)> BinaryOp;
    typedef std::function<Object*(Integer*)> UnaryOp;
    
    Integer();
    Integer(std::string str, int base);
    Integer(int64 i);
    
    std::string asString();
    std::string getTypeData();
    
    int64 getNumber();
  };
  
}; /* namespace lang */

#endif /* BUILTINS_H_ */
