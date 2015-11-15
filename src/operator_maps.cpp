#include "builtins.hpp"

namespace lang {
  OperatorMap opsMap = {
    {Operator("+", 10), {
      {"Integer Integer", boost::any(new Integer::BinaryOp([](Integer* left, Integer* right) {
        return new Integer(left->getNumber() + right->getNumber());
      }))},
      {"Float Float", boost::any(new Float::BinaryOp([](Float* left, Float* right) {
        return new Float(left->getNumber() + right->getNumber());
      }))}
    }},
    {Operator("-", 10), {
      {"Integer Integer", boost::any(new Integer::BinaryOp([](Integer* left, Integer* right) {
        return new Integer(left->getNumber() - right->getNumber());
      }))},
      {"Float Float", boost::any(new Float::BinaryOp([](Float* left, Float* right) {
        return new Float(left->getNumber() - right->getNumber());
      }))}
    }},
  };
} /* namespace lang */