#include "builtins.hpp"

namespace lang {
  OperatorMap opsMap = {
    {Operator("+", 10), {
      {"Integer Integer", boost::any(new Object::BinaryOp([](Object* l, Object* r) {
        Integer* left = dynamic_cast<Integer*>(l);
        Integer* right = dynamic_cast<Integer*>(r);
        return new Integer(left->getNumber() + right->getNumber());
      }))},
      {"Float Float", boost::any(new Object::BinaryOp([](Object* l, Object* r) {
        Float* left = dynamic_cast<Float*>(l);
        Float* right = dynamic_cast<Float*>(r);
        return new Float(left->getNumber() + right->getNumber());
      }))}
    }},
    {Operator("-", 10), {
      {"Integer Integer", boost::any(new Object::BinaryOp([](Object* l, Object* r) {
        Integer* left = dynamic_cast<Integer*>(l);
        Integer* right = dynamic_cast<Integer*>(r);
        return new Integer(left->getNumber() - right->getNumber());
      }))},
      {"Float Float", boost::any(new Object::BinaryOp([](Object* l, Object* r) {
        Float* left = dynamic_cast<Float*>(l);
        Float* right = dynamic_cast<Float*>(r);
        return new Float(left->getNumber() - right->getNumber());
      }))}
    }},
  };
} /* namespace lang */