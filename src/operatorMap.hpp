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

#define OPERATION [](OperandList operands) -> Object::Link
#define CAST(what, type) PtrUtil<type>::dynPtrCast(what)

// Maps an operator name to the operations that use it
std::unordered_map<std::string, Operations> operatorMap {
  {"Add", {
    {{"Integer", "Integer"}, OPERATION {
      int64 result = CAST(operands[0], Integer)->getInt() + CAST(operands[1], Integer)->getInt();
      return PtrUtil<Integer>::make(result);
    }}
    
  }}
};

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
