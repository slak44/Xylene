#ifndef OPERATOR_MAPS_HPP_
#define OPERATOR_MAPS_HPP_

#include <string>
#include <vector>
#include <unordered_map>
#include <boost/any.hpp>

#include "global.hpp"
#include "tokens.hpp"
#include "operators.hpp"
#include "builtins.hpp"
#include "nodes.hpp"

namespace lang {
  typedef std::unordered_map<lang::Operator, std::unordered_map<std::string, boost::any>, lang::OperatorHash> OperatorMap;
  extern OperatorMap opsMap;

  #define Q(x) #x
  #define QUOTE(x) Q(x)

  #define MAKE_UNARY_OP(type, returnData) \
  QUOTE(type), boost::any(new Object::UnaryOp([](Object* op) {\
    auto operand = dynamic_cast<type*>(op);\
    return (returnData);\
  }))

  #define MAKE_BINARY_OP(type1, type2, returnData) \
  QUOTE(type1 type2), boost::any(new Object::BinaryOp([](Object* l, Object* r) {\
    auto left = dynamic_cast<type1*>(l);\
    auto right = dynamic_cast<type2*>(r);\
    return (returnData);\
  }))
  
  #define MAKE_ASSIGNMENT_OP(assignmentOperation, operationPrecedence) \
  {Operator(QUOTE(assignmentOperation=), 1, ASSOCIATE_FROM_RIGHT, BINARY), {\
    {"Variable Object", boost::any(new Object::BinaryOp([](Object* l, Object* r) {\
      auto left = dynamic_cast<Variable*>(l);\
      auto result = runOperator(Operator(QUOTE(assignmentOperation), operationPrecedence), left->read(), r);\
      left->assign(result);\
      return result;\
    }))}\
  }}
  
  #define MAKE_MUTATING_UNARY_OP(operator, sign, returnModified) \
  operator, {\
    {"Variable", boost::any(new Object::UnaryOp([](Object* op) {\
      auto operand = dynamic_cast<Variable*>(op);\
      Object* original;\
      if (!returnModified) original = operand->read();\
      auto result = runOperator(Operator(QUOTE(sign), 10), operand->read(), dynamic_cast<Object*>(new Integer(1)));\
      operand->assign(result);\
      return (returnModified) ? result : original;\
    }))}\
  }

  #define EXPAND_NUMERIC_OPS(operator) \
  {MAKE_BINARY_OP(Integer, Integer, new Integer(left->getNumber() operator right->getNumber()) )},\
  {MAKE_BINARY_OP(Float, Float, new Float(left->getNumber() operator right->getNumber()) )},\
  {MAKE_BINARY_OP(Integer, Float, new Float(left->getNumber() operator right->getNumber()) )},\
  {MAKE_BINARY_OP(Float, Integer, new Float(left->getNumber() operator right->getNumber()) )}

  #define EXPAND_COMPARISON_OPS(operator) \
  {MAKE_BINARY_OP(Integer, Integer, new Boolean(left->getNumber() operator right->getNumber()) )},\
  {MAKE_BINARY_OP(Float, Float, new Boolean(left->getNumber() operator right->getNumber()) )},\
  {MAKE_BINARY_OP(Integer, Float, new Boolean(left->getNumber() operator right->getNumber()) )},\
  {MAKE_BINARY_OP(Float, Integer, new Boolean(left->getNumber() operator right->getNumber()) )}

  Variable* resolveNameFrom(ASTNode* localNode, std::string identifier);
  Object* runOperator(ExpressionChildNode* operatorNode);
}; /* namespace lang */

#endif /* OPERATOR_MAPS_HPP_ */