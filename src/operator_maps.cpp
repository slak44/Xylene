#include "builtins.hpp"

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

namespace lang {
  OperatorMap opsMap = {
    {Operator("=", 1, ASSOCIATE_FROM_RIGHT, BINARY), {
      {"Variable Object", boost::any(new Object::BinaryOp([](Object* l, Object* r) {
        auto left = dynamic_cast<Variable*>(l);
        left->assign(r);
        return r;
      }))}
    }},
    // Comparison operators
    {Operator("<=", 8), {
      EXPAND_COMPARISON_OPS(<=)
    }},
    {Operator("<", 8), {
      EXPAND_COMPARISON_OPS(<)
    }},
    {Operator(">=", 8), {
      EXPAND_COMPARISON_OPS(>=)
    }},
    {Operator(">", 8), {
      EXPAND_COMPARISON_OPS(>)
    }},
    {Operator("==", 7), {
      EXPAND_COMPARISON_OPS(==),
      {MAKE_BINARY_OP(String, String, new Boolean(left->toString() == right->toString()) )},
      {MAKE_BINARY_OP(Boolean, Boolean, new Boolean(left->value() == right->value()) )}
    }},
    {Operator("!=", 7), {
      EXPAND_COMPARISON_OPS(!=),
      {MAKE_BINARY_OP(String, String, new Boolean(left->toString() != right->toString()) )},
      {MAKE_BINARY_OP(Boolean, Boolean, new Boolean(left->value() != right->value()) )}
    }},
    // Arithmetic operators
    {Operator("+", 10), {
      {MAKE_BINARY_OP(String, String, new String(left->toString() + right->toString()) )}, // String concatenation
      EXPAND_NUMERIC_OPS(+)
    }},
    {Operator("-", 10), {
      EXPAND_NUMERIC_OPS(-)
    }},
    {Operator("*", 11), {
      EXPAND_NUMERIC_OPS(*)
    }},
    {Operator("/", 11), {
      EXPAND_NUMERIC_OPS(/)
    }},
    {Operator("%", 11), {
      {MAKE_BINARY_OP(Integer, Integer, new Integer(left->getNumber() % right->getNumber()) )}
    }},
    // Bitwise operators
    {Operator(">>", 9), {
      {MAKE_BINARY_OP(Integer, Integer, new Integer(left->getNumber() >> right->getNumber()) )}
    }},
    {Operator("<<", 9), {
      {MAKE_BINARY_OP(Integer, Integer, new Integer(left->getNumber() << right->getNumber()) )}
    }},
    {Operator("&", 6), {
      {MAKE_BINARY_OP(Integer, Integer, new Integer(left->getNumber() & right->getNumber()) )}
    }},
    {Operator("^", 5), {
      {MAKE_BINARY_OP(Integer, Integer, new Integer(left->getNumber() ^ right->getNumber()) )}
    }},
    {Operator("|", 4), {
      {MAKE_BINARY_OP(Integer, Integer, new Integer(left->getNumber() | right->getNumber()) )}
    }},
    {Operator("~", 12, ASSOCIATE_FROM_RIGHT, UNARY), {
      {MAKE_UNARY_OP(Integer, new Integer(~operand->getNumber()) )}
    }},
    // Logical operators
    {Operator("!", 12, ASSOCIATE_FROM_RIGHT, UNARY), {
      {MAKE_UNARY_OP(Boolean, new Boolean(!operand->value()) )},
      {MAKE_UNARY_OP(Integer, new Boolean(!operand->getNumber()) )}
    }},
    {Operator("&&", 3), {
      {MAKE_BINARY_OP(Boolean, Boolean, new Boolean(left->value() && right->value()) )},
      {MAKE_BINARY_OP(Integer, Boolean, new Boolean(left->getNumber() && right->value()) )},
      {MAKE_BINARY_OP(Boolean, Integer, new Boolean(left->value() && right->getNumber()) )},
      {MAKE_BINARY_OP(Integer, Integer, new Boolean(left->getNumber() && right->getNumber()) )}
    }},
    {Operator("||", 2), {
      {MAKE_BINARY_OP(Boolean, Boolean, new Boolean(left->value() || right->value()) )},
      {MAKE_BINARY_OP(Integer, Boolean, new Boolean(left->getNumber() || right->value()) )},
      {MAKE_BINARY_OP(Boolean, Integer, new Boolean(left->value() || right->getNumber()) )},
      {MAKE_BINARY_OP(Integer, Integer, new Boolean(left->getNumber() || right->getNumber()) )}
    }},
    // Other unary ops
    {Operator("+", 12, ASSOCIATE_FROM_RIGHT, UNARY), {
      {MAKE_UNARY_OP(Integer, new Integer(+operand->getNumber()) )},
      {MAKE_UNARY_OP(Float, new Float(+operand->getNumber()) )}
    }},
    {Operator("-", 12, ASSOCIATE_FROM_RIGHT, UNARY), {
      {MAKE_UNARY_OP(Integer, new Integer(-operand->getNumber()) )},
      {MAKE_UNARY_OP(Float, new Float(-operand->getNumber()) )}
    }},
    {Operator("++", 12, ASSOCIATE_FROM_RIGHT, UNARY), {
      {MAKE_UNARY_OP(Integer, new Integer(operand->getNumber() + 1) )},
      {MAKE_UNARY_OP(Float, new Float(operand->getNumber() + 1) )}
    }},
    {Operator("--", 12, ASSOCIATE_FROM_RIGHT, UNARY), {
      {MAKE_UNARY_OP(Integer, new Integer(operand->getNumber() - 1) )},
      {MAKE_UNARY_OP(Float, new Float(operand->getNumber() - 1) )}
    }},
    // TODO: postfix ops
    // TODO: other assignment ops
    // TODO: member access op
    // TODO: maybe comma op
  };
} /* namespace lang */