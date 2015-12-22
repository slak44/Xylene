#ifndef NODES_H_
#define NODES_H_

#include <vector>
#include <string>
#include <stdexcept>
#include <climits>
#include <cfloat>

#include "global.hpp"
#include "operators.hpp"
#include "tokens.hpp"
#include "builtins.hpp"

namespace lang {
  class ASTNode {
  protected:
    ASTNode* parent = nullptr;
    std::vector<ASTNode*> children {};
    Scope local;
    int lines = -1;
  public:
    ASTNode();
    ASTNode(int lines);
    virtual ~ASTNode();
    
    virtual void addChild(ASTNode* child);
    std::vector<ASTNode*>& getChildren(); // TODO: make some subclasses overload this, too many dynamic_casts
    void setParent(ASTNode* parent);
    ASTNode* getParent();
    
    Scope* getScope();
    Scope* getParentScope();
    
    void setLineNumber(int lines);
    int getLineNumber();
    
    virtual std::string getNodeType();
    virtual void printTree(int level);
  };
  typedef std::vector<ASTNode*> ChildrenNodes;
  
  class SingleChildNode : public ASTNode {
  public:
    SingleChildNode();
    
    void addChild(ASTNode* child);
    ASTNode* getChild(); // TODO: make some subclasses overload this, too many dynamic_casts
    
    std::string getNodeType();
    void printTree(int level);
  };
  
  class BlockNode : public ASTNode {
  public:
    BlockNode();
    
    std::string getNodeType();
    void printTree(int level);
  };
  
  class DeclarationNode : public SingleChildNode {
  public:
    TypeList typeNames;
    Token identifier;
    
    DeclarationNode(TypeList typeNames, Token identifier);
    void addChild(ASTNode* child);
    
    std::string getNodeType();
    void printTree(int level);
  };
  
  typedef std::vector<std::pair<std::string, Variable*> > Arguments;
  class NativeBlockNode;
  class FunctionNode : public BlockNode {
    friend class NativeBlockNode;
  private:
    std::string name;
    Arguments* defaultArguments;
    TypeList returnTypes;
  public:
    FunctionNode(std::string name, Arguments* arguments, TypeList returnTypes);
    void addChild(ASTNode* child);
    
    std::string getNodeType();
    void printTree(int level);
    
    std::string getName();
    Arguments* getArguments();
    TypeList getReturnTypes();
  };
  
  class ExpressionNode : public SingleChildNode {
  private:
    static std::vector<TokenType> validOperandTypes;
    static std::vector<TokenType> possibleFunctionTypes;
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
  
  class ExpressionChildNode : public ASTNode {
  public:
    Token t;
    ExpressionChildNode(Token operand);
    ExpressionChildNode(Token op, std::vector<Token>& operands);
    
    std::string getNodeType();
    void printTree(int level);
  };
  
  class ConditionalNode : public BlockNode {
  private:
    int block = 1;
  public:
    ConditionalNode(ExpressionNode* condition, BlockNode* trueBlock, BlockNode* falseBlock);
    
    ExpressionNode* getCondition();
    BlockNode* getTrueBlock();
    BlockNode* getFalseBlock();
    
    void addChild(ASTNode* child);
    void nextBlock();
   
    std::string getNodeType();
    void printTree(int level);
  };
  
  class WhileNode : public BlockNode {
  public:
    WhileNode(ExpressionNode* condition, BlockNode* loop);
    
    ExpressionNode* getCondition();
    BlockNode* getLoopNode();
    
    void addChild(ASTNode* child);
    
    std::string getNodeType();
    void printTree(int level);
  };
  
  Variable* resolveNameFrom(ASTNode* localNode, std::string identifier);
} /* namespace lang */

#endif /* NODES_H_ */
