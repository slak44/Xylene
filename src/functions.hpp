#ifndef FUNCTIONS_HPP_
#define FUNCTIONS_HPP_

#include <string>

#include "global.hpp"
#include "builtins.hpp"
#include "nodes.hpp"

namespace lang {
  class Function : public Object {
  private:
    FunctionNode* functionCode = nullptr;
  public:
    Function(FunctionNode* node);
    
    bool isTruthy();
    std::string toString();
    std::string getTypeData();
    
    FunctionNode* getFNode();
  };
  
} /* namespace lang */

#endif /* FUNCTIONS_HPP_ */
