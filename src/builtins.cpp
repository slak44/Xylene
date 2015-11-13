#include "builtins.hpp"

namespace builtins {

std::map<ops::Operator*, void*> initializeOperatorMap(std::vector<void*> funcs) {
  if (funcs.size() < ops::opList.size()) {
    print("Warning: Operator function vector does not match number of operators, filling with nullptr.");
    funcs.resize(ops::opList.size(), nullptr);
  }
  std::map<ops::Operator*, void*> map;
  for (uint64 i = 0; i < ops::opList.size(); ++i) {
    map.insert({
      &ops::opList[i], funcs[i]
    });
  }
  return map;
}

Object::Object() {
}

Object::~Object() {
  
}

std::string Object::asString() {
  return std::to_string((int64) this);
}

//std::map<ops::Operator, void*> Object::getOperations() {
//  return std::map<ops::Operator, void*>();
//}

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

std::map<ops::Operator*, void*> Integer::operators = initializeOperatorMap({
  new std::function<Integer*(Integer, Integer)>([](Integer left, Integer right) {
    return new Integer(left.internal >> right.internal);
  })
});

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


//std::map<ops::Operator, void*> Integer::getOperations() {
//  return std::map<ops::Operator*, void*> {
//    {ops::Operator("+", 10), *([](Integer a, Integer b) {return a + b;})}
//  };
//}
//
//bool Integer::operator==(const Integer& right) {return this->internal == right.internal;}
//bool Integer::operator!=(const Integer& right) {return !operator==(right);}
//bool Integer::operator< (const Integer& right) {return this->internal < right.internal;}
//bool Integer::operator> (const Integer& right) {return this->internal > right.internal;}
//bool Integer::operator>=(const Integer& right) {return !operator<(right);}
//bool Integer::operator<=(const Integer& right) {return !operator>(right);}
//
//Integer Integer::operator+(const Integer& right) {return this->internal + right.internal;};
//Integer Integer::operator-(const Integer& right) {return this->internal - right.internal;};
//Integer Integer::operator*(const Integer& right) {return this->internal * right.internal;};
//Integer Integer::operator/(const Integer& right) {return this->internal / right.internal;};
//Integer Integer::operator%(const Integer& right) {return this->internal % right.internal;};
//
//Integer Integer::operator>>(const Integer& right) {return this->internal >> right.internal;};
//Integer Integer::operator<<(const Integer& right) {return this->internal << right.internal;};
//
//Integer Integer::operator--()    {return --(this->internal);};
//Integer Integer::operator++()    {return ++(this->internal);};
//Integer Integer::operator--(int) {return (this->internal)--;};
//Integer Integer::operator++(int) {return (this->internal)++;};
//
//Integer Integer::operator~() {return ~(this->internal);};
//
//Integer Integer::operator&(const Integer& right) {return this->internal & right.internal;};
//Integer Integer::operator^(const Integer& right) {return this->internal ^ right.internal;};
//Integer Integer::operator|(const Integer& right) {return this->internal | right.internal;};

};
