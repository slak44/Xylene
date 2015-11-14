#include "builtins.hpp"

namespace lang {

OperatorMap initializeOperatorMap(std::vector<void*> funcs) {
  if (funcs.size() < opList.size()) {
    print("Warning: Operator function vector does not match number of operators, filling with nullptr.\n");
    funcs.resize(opList.size(), nullptr);
  }
  OperatorMap map;
  for (uint64 i = 0; i < opList.size(); ++i) {
    map.insert({
      opList[i], funcs[i]
    });
  }
  return map;
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

OperatorMap Float::operators = initializeOperatorMap({
  new Float::BinaryOp([](Float* left, Float* right) {
    throw TypeError("Attempt to use bitwise operations on float value.\n");
  }),
  new Float::BinaryOp([](Float* left, Float* right) {
    throw TypeError("Attempt to use bitwise operations on float value.\n");
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
  
  new Float::UnaryOp([](Float* op) {
    return new Float((op->internal)--);
  }),
  new Float::UnaryOp([](Float* op) {
    return new Float((op->internal)++);
  }),
  
  nullptr, // not implemented
  
  new Float::UnaryOp([](Float* op) {
    return new Float(--(op->internal));
  }),
  new Float::UnaryOp([](Float* op) {
    return new Float(++(op->internal));
  }),
  new Float::UnaryOp([](Float* op) {
    return new Float(-(op->internal));
  }),
  new Float::UnaryOp([](Float* op) {
    return new Float(+(op->internal));
  }),
  new Float::UnaryOp([](Float* op) {
    throw TypeError("Attempt to use bitwise operations on float value.\n");
  }),
  nullptr, // not implemented
  
  new Float::BinaryOp([](Float* left, Float* right) {
    return new Float(left->internal * right->internal);
  }),
  new Float::BinaryOp([](Float* left, Float* right) {
    return new Float(left->internal / right->internal);
  }),
  new Float::BinaryOp([](Float* left, Float* right) {
    return new Float(std::fmod(left->internal, right->internal));
  }),
  
  new Float::BinaryOp([](Float* left, Float* right) {
    return new Float(left->internal + right->internal);
  }),
  new Float::BinaryOp([](Float* left, Float* right) {
    return new Float(left->internal - right->internal);
  }),
  
  nullptr, // not implemented
  nullptr, // not implemented
  
  new Float::BinaryOp([](Float* left, Float* right) {
    throw TypeError("Attempt to use bitwise operations on float value.\n");
  }),
  new Float::BinaryOp([](Float* left, Float* right) {
    throw TypeError("Attempt to use bitwise operations on float value.\n");
  }),
  new Float::BinaryOp([](Float* left, Float* right) {
    throw TypeError("Attempt to use bitwise operations on float value.\n");
  }),
  
  nullptr  // not implemented
});

} /* namespace lang */