#include "functions.hpp"
namespace lang {
  Function::Function(FunctionNode* node): functionCode(node) {}

  bool Function::isTruthy() {return false;}
  std::string Function::toString() {return "Function";}
  std::string Function::getTypeData() {return "Function";}
  
  FunctionNode* Function::getFNode() {return functionCode;}
  
  NativeBlockNode::NativeBlockNode(std::function<void(ASTNode*)> nativeCode): nativeCode(nativeCode) {}
  
  std::string NativeBlockNode::getNodeType() {return "NativeBlockNode";}
  void NativeBlockNode::addChild(ASTNode* child) {} // Does nothing on purpose
  std::vector<ASTNode*>& NativeBlockNode::getChildren() {
    return this->children; // This is perpetually empty
  }
  
  void NativeBlockNode::run(ASTNode* funcScope) {
    nativeCode(funcScope);
  }
  
  void NativeBlockNode::setSelfInFunction(FunctionNode* fNode) {
    fNode->children[0] = this;
  }
} /* namespace lang */
