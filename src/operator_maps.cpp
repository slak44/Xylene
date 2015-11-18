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

namespace lang {
  OperatorMap opsMap = {
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
    {Operator("~", 12), {
      {MAKE_UNARY_OP(Integer, new Integer(~operand->getNumber()) )}
    }},
    // Other unary ops
    // TODO: Postfix ops impl
  };
} /* namespace lang */