#ifndef AST_HPP
#define AST_HPP

#include <string>
#include <vector>
#include <memory>
#include <typeinfo>

#include "utils/util.hpp"
#include "utils/error.hpp"
#include "utils/typeInfo.hpp"
#include "token.hpp"

class ASTVisitor;
using ASTVisitorLink = PtrUtil<ASTVisitor>::Link;

class ASTNode: public std::enable_shared_from_this<ASTNode> {
public:
  using Link = std::shared_ptr<ASTNode>;
  using WeakLink = std::weak_ptr<ASTNode>;
  using Children = std::vector<Link>;
protected:
  WeakLink parent = WeakLink();
  Children children {};
  uint64 lineNumber = 0;
  
  static inline void printIndent(uint level) {
    for (uint i = 0; i < level; i++) print("  ");
  }
public:
  ASTNode(uint64 lineNumber = 0);
  virtual ~ASTNode();
  
  virtual void addChild(Link child);
  
  Children getChildren() const;
  Link at(int64 pos) const;
  
  void setParent(WeakLink newParent);
  WeakLink getParent() const;
  
  void setLineNumber(uint64 newLineNumber);
  uint64 getLineNumber() const;
  
  virtual void printTree(uint level) const;
  
  virtual bool operator==(const ASTNode& rhs) const;
  virtual bool operator!=(const ASTNode& rhs) const;
  
  virtual void visit(ASTVisitorLink visitor) = 0;
};

class BlockNode: public ASTNode {
public:
  void printTree(uint level) const;
  
  void visit(ASTVisitorLink visitor);
};

class ExpressionNode: public ASTNode {
private:
  Token tok = Token(UNPROCESSED, 0);
public:
  ExpressionNode(Token token);
  
  std::shared_ptr<ExpressionNode> at(int64 pos) const;
  Token getToken() const;
  
  void printTree(uint level) const;
  
  bool operator==(const ASTNode& rhs) const;
  bool operator!=(const ASTNode& rhs) const;
  
  void visit(ASTVisitorLink visitor);
};

template<typename T, typename std::enable_if<std::is_base_of<ASTNode, T>::value>::type* = nullptr>
struct Node: public PtrUtil<T> {
  typedef typename PtrUtil<T>::Link Link;
  typedef typename PtrUtil<T>::WeakLink WeakLink;
  
  static inline bool isSameType(ASTNode::Link node) {
    return typeid(T) == typeid(*node);
  }
  
  static inline Link dynPtrCast(ASTNode::Link node) {
    return std::dynamic_pointer_cast<T>(node);
  }
};

class AST {
private:
  BlockNode root = BlockNode();
public:
  void print() const;
  void setRoot(BlockNode rootNode);
  BlockNode getRoot() const;
  Node<BlockNode>::Link getRootAsLink() const;
  
  bool operator==(const AST& rhs) const;
  bool operator!=(const AST& rhs) const;
};

#define GET_SIG(linkType, nameOf) Node<linkType>::Link get##nameOf() const;
#define SET_SIG(linkType, nameOf) void set##nameOf(std::shared_ptr<linkType> newNode);
#define GET_SET_SIGS(linkType, nameOf) \
GET_SIG(linkType, nameOf) \
SET_SIG(linkType, nameOf)

;

class NoMoreChildrenNode: public ASTNode {
public:
  NoMoreChildrenNode(int childrenCount);
  
  void addChild(Link child);
  
  bool inline notNull(uint childIndex) const {
    return children[childIndex] != nullptr;
  }
};

class DeclarationNode: public NoMoreChildrenNode {
private:
  std::string identifier;
  DefiniteTypeInfo info;
public:
  DeclarationNode(std::string identifier, TypeList typeList);
    
  GET_SET_SIGS(ExpressionNode, Init)
  
  std::string getIdentifier() const;
  DefiniteTypeInfo getTypeInfo() const;
  
  bool isDynamic() const;
  bool hasInit() const;
  
  void printTree(uint level) const;
  
  bool operator==(const ASTNode& rhs) const;
  bool operator!=(const ASTNode& rhs) const;
  
  void visit(ASTVisitorLink visitor);
};

class BranchNode: public NoMoreChildrenNode {
public:
  BranchNode();
  
  GET_SET_SIGS(ExpressionNode, Condition)
  GET_SET_SIGS(BlockNode, SuccessBlock)
  GET_SIG(ASTNode, FailiureBlock)
  SET_SIG(BlockNode, FailiureBlock)
  SET_SIG(BranchNode, FailiureBlock)
  
  void printTree(uint level) const;
  
  void visit(ASTVisitorLink visitor);
};

class LoopNode: public NoMoreChildrenNode {
public:
  LoopNode();
  
  GET_SET_SIGS(DeclarationNode, Init)
  GET_SET_SIGS(ExpressionNode, Condition)
  GET_SET_SIGS(ExpressionNode, Update)
  GET_SET_SIGS(BlockNode, Code)
  
  void printTree(uint level) const;
  
  void visit(ASTVisitorLink visitor);
};

class ReturnNode: public NoMoreChildrenNode {
public:
  ReturnNode();
  
  GET_SET_SIGS(ExpressionNode, Value)
  
  void printTree(uint level) const;
  
  void visit(ASTVisitorLink visitor);
};

#undef GET_SIG
#undef SET_SIG
#undef GET_SET_SIGS

#define PURE_VIRTUAL_VISIT(nodeName) virtual void visit##nodeName(nodeName##Node* node) = 0;

class ASTVisitor {
public:
  PURE_VIRTUAL_VISIT(Block)
  PURE_VIRTUAL_VISIT(Expression)
  PURE_VIRTUAL_VISIT(Declaration)
  PURE_VIRTUAL_VISIT(Branch)
  PURE_VIRTUAL_VISIT(Loop)
  PURE_VIRTUAL_VISIT(Return)
};

#undef PURE_VIRTUAL_VISIT

#endif
