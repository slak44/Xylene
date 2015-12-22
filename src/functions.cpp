#include "functions.hpp"
namespace lang {
  Function::Function(FunctionNode* node): functionCode(node) {}

  bool Function::isTruthy() {return false;}
  std::string Function::toString() {return "Function";}
  std::string Function::getTypeData() {return "Function";}
  
  FunctionNode* Function::getFNode() {return functionCode;}
} /* namespace lang */
