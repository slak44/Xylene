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
  
  class Object : Stringifyable {
  public:
    typedef std::function<Object*(Object*, Object*)> BinaryOp;
    typedef std::function<Object*(Object*)> UnaryOp;
    
    Object();
    virtual ~Object();
    
    virtual std::string toString() = 0;
    virtual std::string getTypeData() = 0; // Pure virtual, derived classes must implement this
  };
  
  class Variable : public Object {
  private:
    Object* internal = nullptr;
  public:
    Variable();
    Variable(Object* obj);
    
    std::string toString();
    std::string getTypeData();
    
    void assign(Object* newObj);
    Object* read();
  };
  
  class String : public Object {
  private:
    std::string internal = "";
  public:
    String();
    String(std::string str);
    
    std::string toString();
    std::string getTypeData();
    
    std::string getString();
  };

  // TODO: Number<T> class, with T = int64 || double64
  
  class Float : public Object {
  private:
    double64 internal = 0.0;
  public:
    Float();
    Float(std::string str);
    Float(double64 f);
    
    std::string toString();
    std::string getTypeData();
    
    double64 getNumber();
  };
  
  class Integer : public Object {
  private:
    int64 internal = 0;
  public:
    Integer();
    Integer(std::string str, int base);
    Integer(int64 i);
    
    std::string toString();
    std::string getTypeData();
    
    int64 getNumber();
  };
  
  inline std::ostream& operator<<(std::ostream& os, Object& obj) { 
    return os << "Object[" << obj.toString() << "]";
  }
  
  void concatenateNames(std::string& result);
  
  template<typename... Args>
  void concatenateNames(std::string& result, Object*& obj, Args&... args) {
    if (obj->getTypeData() == "Variable") obj = dynamic_cast<Variable*>(obj)->read();
    result += obj->getTypeData() + " ";
    concatenateNames(result, args...);
  }
  
  // TODO: check for nullptr in any of the arguments, throw SyntaxError. Get line numbers from higher up, maybe default param
  template<typename... Args>
  Object* runOperator(Operator* op, Args... pr) {
    std::string funSig = "";
    if (op->toString().back() == '=') funSig = "Variable Object";
    else concatenateNames(funSig, pr...);
    // 1. Get a boost::any instance from the OperatorMap
    // 2. Use boost::any_cast to get a pointer to the operator function
    // 3. Dereference pointer and call function
    try {
      auto result = (*boost::any_cast<std::function<Object*(Args...)>*>(opsMap[*op][funSig]))(pr...);
      return result;
    } catch (boost::bad_any_cast& bac) {
      throw TypeError("Cannot find operation for operator '" + op->toString() + "' and operands '" + funSig + "'.\n");
    }
  }
  
}; /* namespace lang */

#endif /* BUILTINS_H_ */
