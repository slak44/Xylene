#include "nodes.hpp"

namespace lang {
  
  void printIndent(int level) {
    for (int i = 0; i < level; ++i) print("  ");
  }
  
  ASTNode::ASTNode() {}
  ASTNode::~ASTNode() {}
  
  void ASTNode::addChild(ASTNode* child) {
    child->setParent(this);
    children.push_back(child);
  }
  
  std::vector<ASTNode*>& ASTNode::getChildren() {
    return children;
  }
  
  void ASTNode::setParent(ASTNode* parent) {
    this->parent = parent;
  }
  ASTNode* ASTNode::getParent() {
    return parent;
  }
  
  void ASTNode::setLineNumber(int lines) {
    this->lines = lines;
  }
  int ASTNode::getLineNumber() {
    return this->lines;
  }
  
  std::string ASTNode::getNodeType() {
    return "ASTNode";
  }
  
  void ASTNode::printTree(int level) {
    printIndent(level);
    print("Plain ASTNode");
    for (auto child : children) {
      child->printTree(level + 1);
    }
  }
  
  SingleChildNode::SingleChildNode() {};
  void SingleChildNode::addChild(ASTNode* child) {
    if (this->getChildren().size() >= 1) throw std::runtime_error("Trying to add more than one child to SingleChildNode.");
    else {
      this->ASTNode::addChild(child);
    }
  }
  
  ASTNode* SingleChildNode::getChild() {
    return children[0];
  }
  
  std::string SingleChildNode::getNodeType() {
    return "SingleChildNode";
  }
  
  void SingleChildNode::printTree(int level) {
    printIndent(level);
    auto child = dynamic_cast<SingleChildNode*>(this->getChild());
    if (child == nullptr) {
      print("SingleChildNode, empty", "\n");
      return;
    }
    print("SingleChildNode with: ", child, "\n");
    child->printTree(level + 1);
  }
  
  DeclarationNode::DeclarationNode(std::string typeName, Token identifier) {
    this->typeName = typeName;
    this->identifier = identifier;
  }
  
  void DeclarationNode::addChild(ASTNode* child) {
    auto node = dynamic_cast<ExpressionNode*>(child);
    if (node == 0) throw std::invalid_argument("DeclarationNode only accepts an ExpressionNode as its child.");
    this->SingleChildNode::addChild(node);
  }
  
  std::string DeclarationNode::getNodeType() {
    return "DeclarationNode";
  }
  
  void DeclarationNode::printTree(int level) {
    printIndent(level);
    print("Declared ", this->typeName, " as ", this->identifier, "\n");
    if (this->getChildren().size() == 1) dynamic_cast<ExpressionNode*>(this->getChild())->printTree(level + 1);
  }
  
  std::vector<TokenType> ExpressionNode::validOperandTypes {
    INTEGER, FLOAT, STRING, BOOLEAN, ARRAY, TYPE, VARIABLE, FUNCTION, UNPROCESSED
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
        Operator current = *static_cast<Operator*>(tokens[i].typeData);
        Operator topOfStack = *static_cast<Operator*>(opStack.back().typeData);
        while (opStack.size() != 0) {
          if ((current.getAssociativity() == ASSOCIATE_FROM_LEFT && current.getPrecedence() <= topOfStack.getPrecedence()) ||
          (current.getAssociativity() == ASSOCIATE_FROM_RIGHT && current.getPrecedence() < topOfStack.getPrecedence())) {
            popToOut();
          } else break;
          // Non-operators (eg parenthesis) will crash on next line, and must break to properly evaluate expression
          if (opStack.back().type != OPERATOR) break;
          // Get new top of stack
          if (opStack.size() != 0) topOfStack = *static_cast<Operator*>(opStack.back().typeData);
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
    ExpressionChildNode* node;
    if (stackCopy.size() >= 1) {
      auto tok = stackCopy.back();
      stackCopy.pop_back();
      node = new ExpressionChildNode(tok, stackCopy);
    } else {
      throw std::runtime_error("Empty expression.\n");
    }

    this->addChild(node);
  }
  
  std::string ExpressionNode::getNodeType() {
    return "ExpressionNode";
  }
  
  void ExpressionNode::printTree(int level) {
    auto top = static_cast<ExpressionChildNode*>(this->getChildren()[0]);
    top->printTree(level);
  }
  
  ExpressionChildNode::ExpressionChildNode(Token operand): t(operand) {};
  ExpressionChildNode::ExpressionChildNode(Token op, std::vector<Token>& operands): t(op) {
    if (operands.size() == 0) return;
    auto arity = static_cast<Operator*>(op.typeData)->getArity();
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
  
  std::string ExpressionChildNode::getNodeType() {
    return "ExpressionChildNode";
  }
  
  void ExpressionChildNode::printTree(int level) {
    printIndent(level);
    print(this->t, "\n");
    for (auto child : this->getChildren()) {
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
  
} /* namespace lang */
