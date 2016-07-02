#ifndef OPERATOR_MAP_HPP
#define OPERATOR_MAP_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

#include "util.hpp"
#include "object.hpp"
#include "operator.hpp"

typedef std::vector<std::string> OperandTypeList; // The types of the operands
typedef std::vector<Object::Link> OperandList; // The list of actual operands
typedef std::function<Object::Link(OperandList)> OperatorFunction; // The code that runs for a specific operand list
typedef std::unordered_map<OperandTypeList, OperatorFunction, VectorHash<std::string>> Operations; // Maps specific operand types to their respective code

/*
  Dereference all PtrUtil<Reference> in the operand list, by replacing them with the value they reference.
  Use if the operator does not mutate the referenced thing.
*/
void dereferenceAll(OperandList& list) {
  std::for_each(ALL(list), [](Object::Link& element) {
    if (element->getTypeName() == "Reference") {
      element = PtrUtil<Reference>::dynPtrCast(element)->getValue();
    }
  });
}

#define OPERATION [](OperandList operands) -> Object::Link
#define CAST(what, type) PtrUtil<type>::dynPtrCast(what)

/*
  Example result for integer addition:
  {{"Integer", "Integer"}, OPERATION {
    return PtrUtil<Integer>::make(CAST(operands[0], Integer)->getValue() + CAST(operands[1], Integer)->getValue());
  }}
*/
#define BINARY_ARITHMETIC_OP(type1, type2, resultType, op) \
{{#type1, #type2}, OPERATION {\
  dereferenceAll(operands);\
  return PtrUtil<resultType>::make(CAST(operands[0], type1)->getValue() op CAST(operands[1], type2)->getValue());\
}}

#define BINARY_ARITHMETIC_SET(op) \
BINARY_ARITHMETIC_OP(Integer, Integer, Integer, op),\
BINARY_ARITHMETIC_OP(Integer, Float, Float, op),\
BINARY_ARITHMETIC_OP(Float, Integer, Float, op),\
BINARY_ARITHMETIC_OP(Float, Float, Float, op)

// Maps an operator name to the operations that use it
std::unordered_map<std::string, Operations> operatorMap {
  {"Add", {
    BINARY_ARITHMETIC_SET(+),
  }},
  {"Substract", {
    BINARY_ARITHMETIC_SET(-),
  }},
  {"Multiply", {
    BINARY_ARITHMETIC_SET(*),
  }},
  {"Divide", {
    BINARY_ARITHMETIC_SET(/),
  }}
};

#undef BINARY_ARITHMETIC_SET
#undef BINARY_ARITHMETIC_OP
#undef CAST
#undef OPERATION

OperandTypeList typeListFrom(OperandList list) {
  OperandTypeList t {};
  t.resize(list.size());
  std::transform(ALL(list), t.begin(), [](const Object::Link& operand) {
    return operand->getTypeName();
  });
  return t;
}

Object::Link executeOperator(OperatorName opName, OperandList list) {
  OperandTypeList tl = typeListFrom(list);
  OperatorFunction func = operatorMap[opName][tl];
  return func(list);
}

#endif
