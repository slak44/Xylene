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
    Function(FunctionNode* node): functionCode(node) {}

    bool isTruthy() {return false;}
    std::string toString() {return "Function";}
    std::string getTypeData() {return "Function";}
    
    FunctionNode* getFNode() {return functionCode;}
  };
  
  class NativeBlockNode : public BlockNode {
  private:
    std::function<void(ASTNode*)> nativeCode = nullptr;
  public:
    NativeBlockNode(std::function<void(ASTNode*)> nativeCode): nativeCode(nativeCode) {}
    
    std::string getNodeType() {return "NativeBlockNode";}
    // Does nothing on purpose
    void addChild(ASTNode* child) {}
    // Will always return an empty vector
    std::vector<ASTNode*>& getChildren() {return this->children;}
    void run(ASTNode* funcScope) {nativeCode(funcScope);}
    void setSelfInFunction(FunctionNode* fNode) {fNode->children[0] = this;}
  };
  
} /* namespace lang */

#endif /* FUNCTIONS_HPP_ */
