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
  
  class NativeBlockNode : public BlockNode {
  private:
    std::function<void(ASTNode*)> nativeCode = nullptr;
  public:
    NativeBlockNode(std::function<void(ASTNode*)> nativeCode);
      
    std::string getNodeType();
    void addChild(ASTNode* child);
    std::vector<ASTNode*>& getChildren();
    
    void run(ASTNode* funcScope);
    void setSelfInFunction(FunctionNode* fNode);
  };
  
} /* namespace lang */

#endif /* FUNCTIONS_HPP_ */
