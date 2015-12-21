#include "functions.hpp"
namespace lang {
  Function::Function(FunctionNode* node):
    functionCode(node),
    functionScope(node->getArguments()),
    returnTypes(node->getReturnTypes()) {}

  bool Function::isTruthy() {return false;}
  std::string Function::toString() {return "Function";}
  std::string Function::getTypeData() {return "Function";}
  
  Object* Function::call(Arguments* arguments) {
    
    return nullptr;
  }
} /* namespace lang */
