#include "builtins.hpp"

namespace builtins {

OperatorMap initializeOperatorMap(std::vector<void*> funcs) {
  if (funcs.size() < ops::opList.size()) {
    print("Warning: Operator function vector does not match number of operators, filling with nullptr.\n");
    funcs.resize(ops::opList.size(), nullptr);
  }
  OperatorMap map;
  for (uint64 i = 0; i < ops::opList.size(); ++i) {
    map.insert({
      ops::opList[i], funcs[i]
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

OperatorMap Integer::operators = initializeOperatorMap({
  new Integer::BinaryOp([](Integer* left, Integer* right) {
    return new Integer(left->internal >> right->internal);
  }),
  new Integer::BinaryOp([](Integer* left, Integer* right) {
    return new Integer(left->internal << right->internal);
  }),
  
  nullptr, // not implemented
  nullptr, // not implemented
  nullptr, // not implemented
  nullptr, // not implemented
  nullptr, // not implemented
  nullptr, // not implemented
  
  nullptr, // not implemented
  nullptr, // not implemented
  nullptr, // not implemented
  nullptr, // not implemented
  nullptr, // not implemented
  nullptr, // not implemented
  nullptr, // not implemented
  nullptr, // not implemented
  nullptr, // not implemented
  nullptr, // not implemented
  nullptr, // not implemented
  
  new Integer::UnaryOp([](Integer* op) {
    return new Integer((op->internal)--);
  }),
  new Integer::UnaryOp([](Integer* op) {
    return new Integer((op->internal)++);
  }),
  
  nullptr, // not implemented
  
  new Integer::UnaryOp([](Integer* op) {
    return new Integer(--(op->internal));
  }),
  new Integer::UnaryOp([](Integer* op) {
    return new Integer(++(op->internal));
  }),
  new Integer::UnaryOp([](Integer* op) {
    return new Integer(-(op->internal));
  }),
  new Integer::UnaryOp([](Integer* op) {
    return new Integer(+(op->internal));
  }),
  new Integer::UnaryOp([](Integer* op) {
    return new Integer(~(op->internal));
  }),
  nullptr, // not implemented
  
  new Integer::BinaryOp([](Integer* left, Integer* right) {
    return new Integer(left->internal * right->internal);
  }),
  new Integer::BinaryOp([](Integer* left, Integer* right) {
    return new Integer(left->internal / right->internal);
  }),
  new Integer::BinaryOp([](Integer* left, Integer* right) {
    return new Integer(left->internal % right->internal);
  }),
  
  new Integer::BinaryOp([](Integer* left, Integer* right) {
    return new Integer(left->internal + right->internal);
  }),
  new Integer::BinaryOp([](Integer* left, Integer* right) {
    return new Integer(left->internal - right->internal);
  }),
  
  nullptr, // not implemented
  nullptr, // not implemented
  
  new Integer::BinaryOp([](Integer* left, Integer* right) {
    return new Integer(left->internal & right->internal);
  }),
  new Integer::BinaryOp([](Integer* left, Integer* right) {
    return new Integer(left->internal ^ right->internal);
  }),
  new Integer::BinaryOp([](Integer* left, Integer* right) {
    return new Integer(left->internal | right->internal);
  }),
  
  nullptr  // not implemented
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

};
