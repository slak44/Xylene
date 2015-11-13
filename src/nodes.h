#ifndef NODES_H_
#define NODES_H_

#include <vector>
#include <string>
#include <stdexcept>
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
  virtual ~ASTNode();
  
  virtual void addChild(ASTNode* child);
  std::vector<ASTNode*>& getChildren();
  void setParent(ASTNode* parent);
  ASTNode* getParent();
  
  virtual std::string getNodeType();
  virtual void printTree(int level);
};

class SingleChildNode: public ASTNode {
public:
  SingleChildNode();
  
  void addChild(ASTNode* child);
  ASTNode* getChild();
  
  std::string getNodeType();
  void printTree(int level);
};

typedef std::vector<ASTNode*> ChildrenNodes;

class DeclarationNode: public SingleChildNode {
public:
  std::string typeName;
  Token identifier;

  DeclarationNode(std::string typeName, Token identifier);
  void addChild(ASTNode* child);
  
  std::string getNodeType();
  void printTree(int level);
};

class ExpressionNode: public SingleChildNode {
private:
  static std::vector<TokenType> validOperandTypes;
  std::vector<Token> opStack = std::vector<Token>();
  std::vector<Token> outStack = std::vector<Token>();
  
  // Moves the top of the opStack to the top of the outStack
  inline void popToOut() {
    outStack.push_back(opStack.back());
    opStack.pop_back();
  }
public:
  ExpressionNode(std::vector<Token>& tokens);
  
  std::vector<Token> getRPNOutput();
  void buildSubtree();
  
  std::string getNodeType();
  void printTree(int level);
};

class ExpressionChildNode: public ASTNode {
public:
  Token t;
  ExpressionChildNode(Token operand);
  ExpressionChildNode(Token op, std::vector<Token>& operands);
  
  std::string getNodeType();
  void printTree(int level);
};


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
