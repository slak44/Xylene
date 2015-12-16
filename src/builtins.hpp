#ifndef BUILTINS_H_
#define BUILTINS_H_

#include <string>
#include <sstream>
#include <unordered_map>
#include <functional>
#include <stdexcept>
#include <cmath>
#include <boost/any.hpp>

#include "global.hpp"

namespace lang {
  class Object : Stringifyable {
  public:
    typedef std::function<Object*(Object*, Object*)> BinaryOp;
    typedef std::function<Object*(Object*)> UnaryOp;
    
    Object();
    virtual ~Object();
    
    virtual bool isTruthy() = 0;
    virtual std::string toString() = 0;
    virtual std::string getTypeData() = 0; // Pure virtual, derived classes must implement this
  };
  
  class Variable : public Object {
  private:
    Object* internal = nullptr;
    std::vector<std::string> types {};
  public:
    Variable();
    Variable(Object* obj);
    
    bool isTruthy();
    std::string toString();
    std::string getTypeData();
    
    void assign(Object* newObj);
    Object* read();
  };
  
  enum Visibility : int {
    PRIVATE, PUBLIC
  };
  
  class Member : public Object {
  private:
    Variable* var = nullptr;
    Visibility access = PRIVATE;
  public:
    Member(Variable* var, Visibility access = PRIVATE);
    
    bool isTruthy();
    std::string toString();
    std::string getTypeData();
    
    Variable* getVariable();
    void setVariable(Variable* var);
    Visibility getVisibility();
  };
  
  typedef std::unordered_map<std::string, Member*> MemberMap;
  
  class Type : public Object {
  private:
    std::string name;
    MemberMap staticMembers;
  public:
    MemberMap initialMap;
    Type(std::string name, MemberMap staticMap, MemberMap map);
    
    bool isTruthy();
    std::string toString();
    std::string getTypeData();
    
    std::string getName();
    Member* getStaticMember(std::string identifier);
  };
  
  class Instance : public Object {
  private:
    Type* type = nullptr;
    MemberMap members;
  public:
    Instance(Type* t);
    
    bool isTruthy();
    std::string toString();
    std::string getTypeData();
    
    Member* getMember(std::string identifier);
  };
  
  class Boolean : public Object {
  private:
    bool internal = false;
  public:
    Boolean();
    Boolean(bool b);
    Boolean(std::string str);
    
    bool isTruthy();
    std::string toString();
    std::string getTypeData();
    
    bool value();
  };
  
  class String : public Object {
  private:
    std::string internal = "";
  public:
    String();
    String(std::string str);
    
    bool isTruthy();
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
    
    bool isTruthy();
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
    
    bool isTruthy();
    std::string toString();
    std::string getTypeData();
    
    int64 getNumber();
  };
  
  typedef std::unordered_map<std::string, Variable*> Scope;
  
  inline std::ostream& operator<<(std::ostream& os, Object& obj) { 
    return os << "Object[" << obj.toString() << "]";
  }
  
}; /* namespace lang */

#endif /* BUILTINS_H_ */
