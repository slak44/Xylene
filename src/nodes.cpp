#include "nodes.hpp"

namespace lang {
  Variable* resolveNameFrom(ASTNode* localNode, std::string identifier) {
    if (localNode == nullptr) return nullptr; // Reached tree root and didn't find anything.
    try {
      return localNode->getScope()->at(identifier);
    } catch(std::out_of_range& oor) {
      return resolveNameFrom(localNode->getParent(), identifier);
    }
  }
  
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
  
  Scope* ASTNode::getScope() {
    return &local;
  }
  
  Scope* ASTNode::getParentScope() {
    return &this->parent->local;
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
  
  DeclarationNode::DeclarationNode(std::vector<std::string> typeNames, Token identifier) {
    this->typeNames = typeNames;
    this->identifier = identifier;
  }
  
  void DeclarationNode::addChild(ASTNode* child) {
    auto node = dynamic_cast<ExpressionNode*>(child);
    if (node == nullptr) throw std::invalid_argument("DeclarationNode only accepts an ExpressionNode as its child.");
    this->SingleChildNode::addChild(node);
  }
  
  std::string DeclarationNode::getNodeType() {
    return "DeclarationNode";
  }
  
  void DeclarationNode::printTree(int level) {
    printIndent(level);
    print("Declared ", this->typeNames, " as ", this->identifier, "\n");
    if (this->getChildren().size() == 1) dynamic_cast<ExpressionNode*>(this->getChild())->printTree(level + 1);
  }
  
  FunctionNode::FunctionNode(std::string name, Arguments* arguments, std::vector<std::string> returnTypes):
    name(name),
    defaultArguments(arguments),
    returnTypes(returnTypes) {}
  
  void FunctionNode::addChild(ASTNode* child) {
    auto node = dynamic_cast<BlockNode*>(child);
    if (node == nullptr) throw std::invalid_argument("FunctionNode only accepts a BlockNode as its child.");
    this->SingleChildNode::addChild(node);
  }
  
  std::string FunctionNode::getNodeType() {return "FunctionNode";}
  
  void FunctionNode::printTree(int level) {
    printIndent(level);
    print("Function " + name, "\n");
    if (this->getChildren().size() == 1) dynamic_cast<BlockNode*>(this->getChild())->printTree(level + 1);
  }
  
  std::string FunctionNode::getName() {
    return name;
  }
  
  Arguments* FunctionNode::getArguments() {
    return defaultArguments;
  }
  
  TypeList FunctionNode::getReturnTypes() {
    return returnTypes;
  }
  
  std::vector<TokenType> ExpressionNode::validOperandTypes {
    INTEGER, FLOAT, STRING, BOOLEAN, ARRAY, TYPE, VARIABLE, FUNCTION, MEMBER
  };
  
  ExpressionNode::ExpressionNode(std::vector<Token>& tokens) {
    for (uint64 i = 0; i < tokens.size(); ++i) {
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
        if (isNewLine(tokens[i])) break;
        if (tokens[i].data != "(" && tokens[i].data != ")") {
          throw Error("Illegal construct in expression: \"" + tokens[i].data + "\"", "SyntaxError", tokens[i].line);
        } else if (tokens[i].data == "(") {
          opStack.push_back(tokens[i]);
        } else if (tokens[i].data == ")") {
          while (opStack.size() != 0 && opStack.back().data != "(") popToOut();
          if (opStack.back().data != "(") throw Error("Mismatched parenthesis in expression", "SyntaxError", outStack.back().line);
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
      node->setLineNumber(this->lines);
    } else {
      throw std::runtime_error("Empty expression.\n");
    }
    this->children.clear();
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
        branch->setLineNumber(op.line);
        this->addChild(branch);
      } else {
        operands.pop_back();
        auto leaf = new ExpressionChildNode(next);
        leaf->setLineNumber(op.line);
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
  
  BlockNode::BlockNode() {}
  
  std::string BlockNode::getNodeType() {
    return "BlockNode";
  }
  
  void BlockNode::printTree(int level) {
    printIndent(level);
    print("BlockNode\n");
    for (auto node : children) {
      node->printTree(level + 1);
    }
  }
  
  ConditionalNode::ConditionalNode(ExpressionNode* condition, BlockNode* trueBlock, BlockNode* falseBlock) {
    this->children.push_back(condition);
    this->children.push_back(trueBlock);
    this->children.push_back(falseBlock);
    condition->setParent(this);
    trueBlock->setParent(this);
    falseBlock->setParent(this);
  }
  
  ExpressionNode* ConditionalNode::getCondition() {return dynamic_cast<ExpressionNode*>(children[0]);}
  BlockNode* ConditionalNode::getTrueBlock() {return dynamic_cast<BlockNode*>(children[1]);}
  BlockNode* ConditionalNode::getFalseBlock() {return dynamic_cast<BlockNode*>(children[2]);}
  
  void ConditionalNode::addChild(ASTNode* child) {
    children[this->block]->addChild(child);
  }
  
  void ConditionalNode::nextBlock() {
    static bool wasCalled = false;
    if (!wasCalled) {
      wasCalled = true;
      this->block++;
    } else throw Error("Multiple else statements", "SyntaxError", children[2]->getLineNumber());
  }
  
  std::string ConditionalNode::getNodeType() {
    return "ConditionalNode";
  }
  
  void ConditionalNode::printTree(int level) {
    printIndent(level);
    print("Condition\n");
    this->getCondition()->printTree(level + 1);
  }
  
  WhileNode::WhileNode(ExpressionNode* condition, BlockNode* loop) {
    this->children.push_back(condition);
    this->children.push_back(loop);
    condition->setParent(this);
    loop->setParent(this);
  }
  
  ExpressionNode* WhileNode::getCondition() {return dynamic_cast<ExpressionNode*>(children[0]);}
  BlockNode* WhileNode::getLoopNode() {return dynamic_cast<BlockNode*>(children[1]);}
  
  void WhileNode::addChild(ASTNode* child) {
    children[1]->addChild(child);
  }
  
  std::string WhileNode::getNodeType() {
    return "WhileNode";
  }
  
  void WhileNode::printTree(int level) {
    printIndent(level);
    print("While Condition\n");
    this->getCondition()->printTree(level + 1);
  }

  AST::AbstractSyntaxTree() {
    Type* integerType = new Type(std::string("Integer"), {
      // Static members
      {"MAX_VALUE", new Member(new Variable(new Integer(LLONG_MAX), {}), PUBLIC)},
      {"MIN_VALUE", new Member(new Variable(new Integer(LLONG_MIN), {}), PUBLIC)}
    }, {
      // Instance members
      // TODO add members
    });
    (*root.getScope())[std::string("Integer")] = new Variable(integerType, {}); // Do not allow assignment by not specifying any allowed types for the Variable
    Type* floatType = new Type(std::string("Float"), {
      // Static members
      {"MAX_VALUE", new Member(new Variable(new Float(FLT_MAX), {}), PUBLIC)},
      {"MIN_VALUE", new Member(new Variable(new Float(FLT_MIN), {}), PUBLIC)}
    }, {
      // Instance members
      // TODO add members
    });
    (*root.getScope())[std::string("Float")] = new Variable(floatType, {});
    Type* stringType = new Type(std::string("String"), {
      // Static members
    }, {
      // Instance members
      // TODO add members
    });
    (*root.getScope())[std::string("String")] = new Variable(stringType, {});
    Type* booleanType = new Type(std::string("Boolean"), {
      // Static members
    }, {
      // Instance members
      // TODO add members
    });
    (*root.getScope())[std::string("Boolean")] = new Variable(booleanType, {});
    Type* functionType = new Type(std::string("Function"), {
      // Static members
    }, {
      // Instance members
      // TODO add members
    });
    (*root.getScope())[std::string("Function")] = new Variable(functionType, {});
  }
  
  void AST::addRootChild(ASTNode* node) {
    root.addChild(node);
  }
  
  ChildrenNodes AST::getRootChildren() {
    return root.getChildren();
  }
  
} /* namespace lang */
