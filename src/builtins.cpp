#include "builtins.hpp"

namespace lang {
  Object::~Object() {}
  
  Variable::Variable() {}
  Variable::Variable(Object* obj, TypeList types):
    internal(obj),
    types(types) {
    if (obj != nullptr) currentType = obj->getTypeData();
  }
  
  bool Variable::isTruthy() {return this->read()->isTruthy();}
  std::string Variable::toString() {
    std::stringstream ss; ss << this;
    return std::string("Variable ") + ss.str() + ":" + (this->internal == nullptr ? "Unassigned" : this->internal->toString());
  }
  std::string Variable::getTypeData() {return "Variable";}
  
  void Variable::assign(Object* newObj) {
    if (newObj == nullptr) throw Error("Attemptted assignment of nullptr on " + this->toString(), "NullPointerError", -2); // TODO: line numbers
    if (!contains(std::string("define"), types) && !contains(newObj->getTypeData(), types))
      throw Error("Invalid assignment of type " + newObj->getTypeData(), "TypeError", -2); // TODO: line numbers
    currentType = newObj->getTypeData();
    this->internal = newObj;
  }
  Object* Variable::read() {
    return this->internal;
  }
  std::string Variable::getCurrentType() {
    return this->currentType;
  }
  TypeList Variable::getTypes() {
    return this->types;
  }
  
  Member::Member(Variable* var, Visibility access):
    var(var),
    access(access) {
  }
  
  bool Member::isTruthy() {return this->var->isTruthy();}
  std::string Member::toString() {
    return "Member with " + this->var->toString() + " and visibility " + std::to_string(this->access);
  }
  std::string Member::getTypeData() {return "Member";}
  
  Variable* Member::getVariable() {return this->var;}
  void Member::setVariable(Variable* var) {this->var = var;}
  Visibility Member::getVisibility() {return this->access;}
  
  Type::Type(std::string name, MemberMap staticMap, MemberMap map):
    name(name),
    staticMembers(staticMap),
    initialMap(map) {
  }
  
  bool Type::isTruthy() {return false;}
  std::string Type::toString() {
    return "Type " + this->name;
  }
  std::string Type::getTypeData() {return "Type";}
  
  std::string Type::getName() {return this->name;}
  Member* Type::getStaticMember(std::string identifier) {return staticMembers[identifier];}
  Instance* Type::createInstance() {
    return new Instance(this);
  }
  
  Instance::Instance(Type* t):
    type(t),
    members(t->initialMap) {
  }
  
  bool Instance::isTruthy() {return false;}
  std::string Instance::toString() {
    return "Instance of " + this->type->toString();
  }
  std::string Instance::getTypeData() {return "Instance";}
  
  Member* Instance::getMember(std::string identifier) {
    auto member = this->members[identifier];
    if (member == nullptr) member = this->type->getStaticMember(identifier); // Check for a static member
    if (member == nullptr) throw Error("Could not find member " + identifier, "NullPointerError", -2); // TODO: line number
    if (member->getVisibility() != PUBLIC) throw Error("Member " + identifier + " is not visible", "Error", -2);
    return member;
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
