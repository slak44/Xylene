#include "builtins.hpp"

#define Q(x) #x
#define QUOTE(x) Q(x)

#define MAKE_BINARY_OP(type1, type2, returnData) \
QUOTE(type1 type2), boost::any(new Object::BinaryOp([](Object* l, Object* r) {\
  auto left = dynamic_cast<type1*>(l);\
  auto right = dynamic_cast<type2*>(r);\
  return returnData;\
}))

namespace lang {
  OperatorMap opsMap = {
    {Operator("+", 10), {
      // String concatenation
      {MAKE_BINARY_OP(String, String, new String(left->toString() + right->toString()) )},
      // Operations with numbers
      {MAKE_BINARY_OP(Integer, Integer, new Integer(left->getNumber() + right->getNumber()) )},
      {MAKE_BINARY_OP(Float, Float, new Float(left->getNumber() + right->getNumber()) )},
      {MAKE_BINARY_OP(Integer, Float, new Float(left->getNumber() + right->getNumber()) )},
      {MAKE_BINARY_OP(Float, Integer, new Float(left->getNumber() + right->getNumber()) )}
    }},
    {Operator("-", 10), {
      {MAKE_BINARY_OP(Integer, Integer, new Integer(left->getNumber() - right->getNumber()) )},
      {MAKE_BINARY_OP(Float, Float, new Float(left->getNumber() - right->getNumber()) )},
    }},
  };
} /* namespace lang */