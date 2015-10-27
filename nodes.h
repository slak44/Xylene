#ifndef NODES_H_
#define NODES_H_

#include <vector>
#include <string>
#include <memory>

#include "global.h"
#include "std_is_missing_stuff.h"
#include "operators.h"
#include "tokens.h"

namespace nodes {

class ASTNode {
protected:
  ASTNode* parent = nullptr;
  std::vector<ASTNode*> children {};
public:
  ASTNode();
  void addChild(ASTNode* child);
  std::vector<ASTNode*> getChildren();
  void setParent(ASTNode* parent);
  ASTNode* getParent();
};

typedef std::vector<ASTNode*> ChildrenNodes;

class DeclarationNode: public ASTNode {
public:
  std::string typeName;
  std::string identifier;

  DeclarationNode(std::string typeName, std::string identifier);
};

class ExpressionNode: public ASTNode {
private:
  static std::vector<TokenType> validOperandTypes;
  std::vector<Token> opStack {};
  std::vector<Token> outStack {};
  
  // Moves the top of the opStack to the top of the outStack
  inline void popToOut() {
    outStack.push_back(opStack.back());
    opStack.pop_back();
  }
public:
  ExpressionNode(std::vector<Token>& tokens);
  
  std::vector<Token> getRPNOutput();
  void buildSubtree();
};

class ExpressionChildNode: public ASTNode {
public:
  Token t;
  ExpressionChildNode(Token operand);
  ExpressionChildNode(Token op, std::vector<Token>& operands);
};

inline std::ostream& operator<<(std::ostream& os, ExpressionChildNode& tok) { 
  if (tok.t.type != OPERATOR) return os << "ExpressionChildNode with " << tok.t;
  else {
    for (unsigned long long i = 0; i < tok.getChildren().size(); ++i) {
      operator<<(os, *static_cast<ExpressionChildNode*>(tok.getChildren()[i]));
    }
    return os;
  }
}

class AbstractSyntaxTree {
private:
  ASTNode root = ASTNode();
public:
  AbstractSyntaxTree();
  
  void addRootChild(ASTNode* node);
  ChildrenNodes getRootChildren();
};

typedef nodes::AbstractSyntaxTree AST;

} /* namespace nodes */

#endif /* NODES_H_ */
