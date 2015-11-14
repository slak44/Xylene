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

} // namespace builtins