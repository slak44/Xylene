#include "nodes.h"

namespace nodes {

void printIndent(int level) {
  for (int i = 0; i < level; ++i) print("  ");
}

ASTNode::ASTNode() {}
ASTNode::~ASTNode() {}

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

void ASTNode::printTree(int level) {
  printIndent(level);
  print("Plain ASTNode");
  for (auto child : children) {
    child->printTree(level + 1);
  }
}

DeclarationNode::DeclarationNode(std::string typeName, std::string identifier) {
  this->typeName = typeName;
  this->identifier = identifier;
}

std::vector<TokenType> ExpressionNode::validOperandTypes {
  INTEGER, FLOAT, STRING, ARRAY, TYPE, VARIABLE, FUNCTION, UNPROCESSED
};

ExpressionNode::ExpressionNode(std::vector<Token>& tokens) {
  for (unsigned long long i = 0; i < tokens.size(); ++i) {
    if (EXPRESSION_STEPS) {
      for (auto tok : outStack) print(tok.data, " ");
      print("\n-------\n");
      for (auto tok : opStack) print(tok.data, " ");
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

void ExpressionNode::buildSubtree(void) {
  std::vector<Token> stackCopy(outStack);
  auto tok = stackCopy.back();
  stackCopy.pop_back();
  ExpressionChildNode* node = new ExpressionChildNode(tok, stackCopy);
  this->addChild(node);
}

void ExpressionNode::printTree(int level) {
  auto top = static_cast<ExpressionChildNode*>(this->getChildren()[0]);
  top->printTree(level);
}

ExpressionChildNode::ExpressionChildNode(Token operand): t(operand) {};
ExpressionChildNode::ExpressionChildNode(Token op, std::vector<Token>& operands): t(op) {
  auto arity = static_cast<ops::Operator*>(op.typeData)->getArity();
  for (int i = 0; i < arity; ++i) {
    auto next = operands[operands.size() - 1];
    if (next.type == OPERATOR) {
      operands.pop_back();
      auto branch = new ExpressionChildNode(next, operands);
      this->addChild(branch);
    } else {
      operands.pop_back();
      auto leaf = new ExpressionChildNode(next);
      this->addChild(leaf);
    }
  }
};

void ExpressionChildNode::printTree(int level) {
  printIndent(level);
  print(this->t, "\n");
  for (auto child : children) {
    static_cast<ExpressionChildNode*>(child)->printTree(level + 1);
  }
}

AST::AbstractSyntaxTree() {}

void AST::addRootChild(ASTNode* node) {
  root.addChild(node);
}

ChildrenNodes AST::getRootChildren() {
  return root.getChildren();
}

}
