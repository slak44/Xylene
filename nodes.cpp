#include "nodes.h"

namespace nodes {

ASTNode::ASTNode() {}

void ASTNode::addChild(ASTNode* child) {
  child->setParent(this);
  children.push_back(child);
}
std::vector<ASTNode*> ASTNode::getChildren() {
  return children;
}

void ASTNode::setParent(ASTNode* parent) {
  this->parent = parent;
}
ASTNode* ASTNode::getParent() {
  return parent;
}

DeclarationNode::DeclarationNode(std::string typeName, std::string identifier) {
  this->typeName = typeName;
  this->identifier = identifier;
}

std::vector<TokenType> ExpressionNode::validOperandTypes {
  INTEGER, FLOAT, STRING, ARRAY, TYPE, VARIABLE, FUNCTION, UNPROCESSED
};

ExpressionNode::ExpressionNode(std::vector<Token> tokens) {
  for (unsigned int i = 0; i < tokens.size(); ++i) {
    if (EXPRESSION_STEPS) {
      for (auto tok : outStack) printnb(tok.data << ' ');
      print("\n-------\n");
      for (auto tok : opStack) printnb(tok.data << ' ');
      print("\n=======\n");
    }
    if (contains(tokens[i].type, validOperandTypes)) {
      outStack.push_back(tokens[i]);
    } else if (tokens[i].type == OPERATOR) {
      /* 
       * This prevents crash when there's no operators.
       * Or when it found a parenthesis, because they're not really operators.
       */
      if (opStack.size() == 0 || opStack.back().data == "(") {
        opStack.push_back(tokens[i]);
        continue;
      }
      ops::Operator current = *static_cast<ops::Operator*>(tokens[i].typeData);
      ops::Operator topOfStack = *static_cast<ops::Operator*>(opStack.back().typeData);
      while (opStack.size() != 0) {
        if ((current.getAssociativity() == ops::ASSOCIATE_FROM_LEFT && current <= topOfStack) ||
        (current.getAssociativity() == ops::ASSOCIATE_FROM_RIGHT && current < topOfStack)) {
          popToOut();
        } else break;
        // Non-operators (eg parenthesis) will crash on next line, and must break to properly evaluate expression
        if (opStack.back().type != OPERATOR) break;
        // Get new top of stack
        if (opStack.size() != 0) topOfStack = *static_cast<ops::Operator*>(opStack.back().typeData);
      }
      opStack.push_back(tokens[i]);
    } else if (tokens[i].type == CONSTRUCT) {
      if (tokens[i].data == ";") break;
      if (tokens[i].data != "(" && tokens[i].data != ")") {
        throw SyntaxError("Illegal construct in expression. \"" + tokens[i].data + "\"", tokens[i].line);
      } else if (tokens[i].data == "(") {
        opStack.push_back(tokens[i]);
      } else if (tokens[i].data == ")") {
        while (opStack.size() != 0 && opStack.back().data != "(") popToOut();
        if (opStack.back().data != "(") throw SyntaxError("Mismatched parenthesis in expression", outStack.back().line);
        opStack.pop_back(); // Get rid of "("
        if (opStack.back().type == FUNCTION) popToOut();
      }
    }
  }
  while (opStack.size() > 0) popToOut();
}

std::vector<Token> ExpressionNode::getRPNOutput() {
  return outStack;
}

ExpressionNode::buildSubtree() {
  ExpressionChildNode subroot = ExpressionChildNode(outStack.back());
  this.addChild(subroot);
}

ExpressionChildNode::ExpressionChildNode(Token t): t(t) {};

AST::AbstractSyntaxTree() {}

void AST::addRootChild(ASTNode* node) {
  root.addChild(node);
}
ChildrenNodes AST::getRootChildren() {
  return root.getChildren();
}

}
