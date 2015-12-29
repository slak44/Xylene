#ifndef OBJECTS_HPP_
#define OBJECTS_HPP_

#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <sstream>

#include "global.hpp"

namespace lang {
  typedef std::vector<std::string> TypeList;
  class Object : Stringifyable {
  public:
    typedef std::function<Object*(Object*, Object*)> BinaryOp;
    typedef std::function<Object*(Object*)> UnaryOp;
    
    virtual ~Object() {};
    
    virtual bool isTruthy() = 0;
    virtual std::string toString() = 0;
    virtual std::string getTypeData() = 0; // Pure virtual, derived classes must implement this
  };
  
  class Name : public Object {
  private:
    std::string name;
  public:
    Name(std::string name): name(name) {};
    bool isTruthy() {return false;}
    std::string toString() {return name;}
    std::string getTypeData() {return "Name";}
  };
  
  class Variable : public Object {
  private:
    Object* internal = nullptr;
    std::string currentType = "";
    TypeList types {};
  public:
    Variable() {}
    Variable(Object* obj, TypeList types):
      internal(obj),
      types(types) {
      if (obj != nullptr) currentType = obj->getTypeData();
    }
    
    std::string toString() {
      std::stringstream ss; ss << this;
      return std::string("Variable ") + ss.str() + ": " + (this->internal == nullptr ? "Unassigned" : this->internal->toString());
    }
    bool isTruthy() {return this->read()->isTruthy();}
    std::string getTypeData() {return "Variable";}
    
    void assign(Object* newObj) {
      if (newObj == nullptr) throw Error("Attemptted assignment of nullptr on " + this->toString(), "NullPointerError", -2); // TODO: line numbers
      if (!contains(std::string("define"), types) && !contains(newObj->getTypeData(), types))
        throw Error("Invalid assignment of type " + newObj->getTypeData(), "TypeError", -2); // TODO: line numbers
      currentType = newObj->getTypeData();
      this->internal = newObj;
    }
    Object* read() {return this->internal;}
    std::string getCurrentType() {return this->currentType;}
    TypeList getTypes() {return this->types;}
  };
  
  enum Visibility : int {
    PRIVATE, PUBLIC
  };
  
  class Member : public Object {
  private:
    Variable* var = nullptr;
    Visibility access = PRIVATE;
  public:
    Member(Variable* var, Visibility access = PRIVATE):
      var(var),
      access(access) {
    }
    
    bool isTruthy() {return this->var->isTruthy();}
    std::string toString() {
      return "Member with " + this->var->toString() + " and visibility " + std::to_string(this->access);
    }
    std::string getTypeData() {return "Member";}
    
    Variable* getVariable() {return this->var;}
    void setVariable(Variable* var) {this->var = var;}
    Visibility getVisibility() {return this->access;}
  };
  
  class Instance;
  
  typedef std::unordered_map<std::string, Member*> MemberMap;
  
  class Type : public Object {
    friend class Instance;
  private:
    std::string name;
  protected:
    MemberMap staticMembers;
    MemberMap instanceMembers;
  public:
    Type(std::string name, MemberMap staticMembers, MemberMap instanceMembers):
      name(name),
      staticMembers(staticMembers),
      instanceMembers(instanceMembers) {}
    
    bool isTruthy() {return false;}
    std::string toString() {return "Type " + this->name;}
    std::string getTypeData() {return "Type";}
    
    std::string getName() {return this->name;}
    Member* getStaticMember(std::string identifier) {return staticMembers[identifier];}
  };
  
  class Instance : public Object {
  private:
    Type* type = nullptr;
    MemberMap members;
  public:
    Instance(Type* t):
      type(t),
      members(t->instanceMembers) {
    }
    
    bool isTruthy() {return false;}
    std::string toString() {
      return "Instance of " + this->type->toString();
    }
    std::string getTypeData() {return "Instance";}
    
    Member* getMember(std::string identifier) {
      auto member = this->members[identifier];
      if (member == nullptr) member = this->type->getStaticMember(identifier); // Check for a static member
      if (member == nullptr) throw Error("Could not find member " + identifier, "NullPointerError", -2); // TODO: line number
      if (member->getVisibility() != PUBLIC) throw Error("Member " + identifier + " is not visible", "Error", -2); // TODO: line number
      return member;
    }
  };
  
  typedef std::unordered_map<std::string, Variable*> Scope;
  
  inline std::ostream& operator<<(std::ostream& os, Object& obj) { 
    return os << "Object[" << obj.toString() << "]";
  }
} /* namespace lang */

#endif /* OBJECTS_HPP_ */
