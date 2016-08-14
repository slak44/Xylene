#ifndef AST_HPP
#define AST_HPP

#include <string>
#include <sstream>
#include <vector>
#include <set>
#include <memory>
#include <typeinfo>

#include "utils/error.hpp"
#include "token.hpp"
#include "utils/util.hpp"
#include "operator.hpp"
#include "utils/typeInfo.hpp"

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
  
  static void printIndent(uint level) {
    for (uint i = 0; i < level; i++) print("  ");
  }
public:
  ASTNode(uint64 lineNumber = 0): lineNumber(lineNumber) {}
  virtual ~ASTNode() {};
  
  virtual void addChild(Link child) {
    child->setParent(shared_from_this());
    children.push_back(child);
  }
  
  Children getChildren() const {
    return children;
  }
  
  Link at(int64 pos) const {
    if (pos < 0) {
      pos = children.size() + pos; // Negative indices count from the end of the vector
    }
    if (static_cast<std::size_t>(abs(pos)) > children.size() || pos < 0) {
      throw InternalError("Index out of array bounds", {METADATA_PAIRS, {"index", std::to_string(pos)}});
    }
    return children[pos];
  }
  
  void setParent(WeakLink newParent) {parent = newParent;}
  WeakLink getParent() const {return parent;}
  
  void inline setLineNumber(uint64 newLineNumber) {
    lineNumber = newLineNumber;
  }
  uint64 inline getLineNumber() const {
    return lineNumber;
  }
  
  virtual void printTree(uint level) const {
    printIndent(level);
    println("Base ASTNode");
    for (auto& child : children) child->printTree(level + 1);
  }
  
  virtual bool operator==(const ASTNode& rhs) const {
    if (typeid(*this) != typeid(rhs)) return false;
    if (children.size() != rhs.getChildren().size()) return false;
    for (uint64 i = 0; i < children.size(); i++) {
      if (*(this->at(i)) != *(rhs.at(i))) return false;
    }
    return true;
  }
  
  virtual bool operator!=(const ASTNode& rhs) const {
    return !operator==(rhs);
  }
  
  virtual void visit(ASTVisitorLink visitor) = 0;
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

#define GET_FOR(childIndex, nameOf, linkType) \
Node<linkType>::Link get##nameOf() const {\
  return Node<linkType>::dynPtrCast(children[childIndex]);\
}

#define SET_FOR(childIndex, nameOf, linkType) \
void set##nameOf(std::shared_ptr<linkType> newNode) {children[childIndex] = newNode;}

#define GET_SET_FOR(childIndex, nameOf, linkType) \
GET_FOR(childIndex, nameOf, linkType) \
SET_FOR(childIndex, nameOf, linkType)

#define PRETTY_PRINT_FOR(childIndex, name) \
{\
  printIndent(level + 1);\
  println(#name":");\
  children[childIndex]->printTree(level + 1);\
}

class NoMoreChildrenNode: public ASTNode {
public:
  NoMoreChildrenNode(int childrenCount) {
    children.resize(childrenCount, nullptr);
  }
  
  void addChild(Link child) {
    UNUSED(child);
    throw InternalError("Cannot add children to NoMoreChildrenNode", {METADATA_PAIRS});
  }
  
  bool inline notNull(uint childIndex) const {
    return children[childIndex] != nullptr;
  }
};

class BlockNode: public ASTNode {
public:
  void printTree(uint level) const {
    printIndent(level);
    println("Block Node");
    for (auto& child : children) child->printTree(level + 1);
  }
  bool operator==(const ASTNode& rhs) const {
    return ASTNode::operator==(rhs);
  }
  bool operator!=(const ASTNode& rhs) const {
    return !operator==(rhs);
  }
  
  void visit(ASTVisitorLink visitor);
};

class ExpressionNode: public ASTNode {
private:
  Token tok = Token(UNPROCESSED, 0);
public:
  ExpressionNode() {}
  ExpressionNode(Token token): tok(token) {
    switch (tok.type) {
      case IDENTIFIER:
      case OPERATOR:
      case L_INTEGER:
      case L_FLOAT:
      case L_STRING:
      case L_BOOLEAN:
        break;
      default: throw InternalError("Trying to add unsupported token to ExpressionNode", {METADATA_PAIRS, {"token", token.toString()}});
    }
  }
  
  std::shared_ptr<ExpressionNode> at(int64 pos) {
    return std::dynamic_pointer_cast<ExpressionNode>(ASTNode::at(pos));
  }
  
  Token getToken() const {
    return tok;
  }
  
  void printTree(uint level) const {
    printIndent(level);
    println("Expression Node:", tok);
    for (auto& child : children) child->printTree(level + 1);
  }
  
  bool operator==(const ASTNode& rhs) const {
    if (!ASTNode::operator==(rhs)) return false;
    if (this->tok != dynamic_cast<const ExpressionNode&>(rhs).tok) return false;
    return true;
  }
  bool operator!=(const ASTNode& rhs) const {
    return !operator==(rhs);
  }
  
  void visit(ASTVisitorLink visitor);
};

class DeclarationNode: public NoMoreChildrenNode {
private:
  std::string identifier;
  DefiniteTypeInfo info;
public:
  DeclarationNode(std::string identifier, TypeList typeList):
    NoMoreChildrenNode(1), identifier(identifier), info(typeList) {}
    
  std::string getIdentifier() const {
    return identifier;
  }
  
  DefiniteTypeInfo getTypeInfo() const {
    return info;
  }
  
  bool isDynamic() const {
    return info.isDynamic();
  }
  
  GET_SET_FOR(0, Init, ExpressionNode)
  
  bool hasInit() const {
    return children[0] != nullptr;
  }
  
  void printTree(uint level) const {
    printIndent(level);
    println("Declaration Node: " + identifier + " (" + info.toString() + ")");
    if (children.size() > 0) children[0]->printTree(level + 1);
  }
  
  bool operator==(const ASTNode& rhs) const {
    if (!ASTNode::operator==(rhs)) return false;
    auto decl = dynamic_cast<const DeclarationNode&>(rhs);
    if (this->identifier != decl.identifier) return false;
    if (this->info != decl.info) return false;
    return true;
  }
  bool operator!=(const ASTNode& rhs) const {
    return !operator==(rhs);
  }
  
  void visit(ASTVisitorLink visitor);
};

class BranchNode: public NoMoreChildrenNode {
public:
  BranchNode(): NoMoreChildrenNode(3) {}
  
  GET_SET_FOR(0, Condition, ExpressionNode)
  GET_SET_FOR(1, SuccessBlock, BlockNode)
  GET_FOR(2, FailiureBlock, ASTNode)
  SET_FOR(2, FailiureBlock, BlockNode)
  SET_FOR(2, FailiureBlock, BranchNode)
  
  bool operator==(const ASTNode& rhs) const {
    return ASTNode::operator==(rhs);
  }
  bool operator!=(const ASTNode& rhs) const {
    return !operator==(rhs);
  }
  
  void printTree(uint level) const {
    printIndent(level);
    println("Branch Node:");
    PRETTY_PRINT_FOR(0, Condition)
    PRETTY_PRINT_FOR(1, Success)
    if (notNull(2)) PRETTY_PRINT_FOR(2, Failiure)
  }
  
  void visit(ASTVisitorLink visitor);
};

class LoopNode: public NoMoreChildrenNode {
public:
  LoopNode(): NoMoreChildrenNode(4) {}
  
  GET_SET_FOR(0, Init, DeclarationNode)
  GET_SET_FOR(1, Condition, ExpressionNode)
  GET_SET_FOR(2, Update, ExpressionNode)
  GET_SET_FOR(3, Code, BlockNode)
  
  void printTree(uint level) const {
    printIndent(level);
    println("Loop Node");
    if (notNull(0)) PRETTY_PRINT_FOR(0, Init)
    if (notNull(1)) PRETTY_PRINT_FOR(1, Condition)
    if (notNull(2)) PRETTY_PRINT_FOR(2, Update)
    if (notNull(3)) PRETTY_PRINT_FOR(3, Code)
  }
  bool operator==(const ASTNode& rhs) const {
    return ASTNode::operator==(rhs);
  }
  bool operator!=(const ASTNode& rhs) const {
    return !operator==(rhs);
  }
  
  void visit(ASTVisitorLink visitor);
};

class ReturnNode: public NoMoreChildrenNode {
public:
  ReturnNode(): NoMoreChildrenNode(1) {}
  
  GET_SET_FOR(0, Value, ExpressionNode)
  
  void printTree(uint level) const {
    printIndent(level);
    println("Return Node");
    if (notNull(0)) PRETTY_PRINT_FOR(0, Value)
  }
  bool operator==(const ASTNode& rhs) const {
    return ASTNode::operator==(rhs);
  }
  bool operator!=(const ASTNode& rhs) const {
    return !operator==(rhs);
  }
  
  void visit(ASTVisitorLink visitor);
};

class AST {
private:
  BlockNode root = BlockNode();
public:
  bool operator==(const AST& rhs) const {
    if (this->root != rhs.root) return false;
    return true;
  }
  
  bool operator!=(const AST& rhs) const {
    return !operator==(rhs);
  }
  
  void print() const {
    root.printTree(0);
  }
  
  void setRoot(BlockNode rootNode) {
    root = rootNode;
  }
  
  BlockNode getRoot() const {
    return root;
  }
  
  Node<BlockNode>::Link getRootAsLink() const {
    return Node<BlockNode>::make(root);
  }
};

#define VISITOR_IMPL(nodeName) virtual void visit##nodeName(nodeName##Node* node) = 0;

class ASTVisitor {
public:
  VISITOR_IMPL(Block)
  VISITOR_IMPL(Expression)
  VISITOR_IMPL(Declaration)
  VISITOR_IMPL(Branch)
  VISITOR_IMPL(Loop)
  VISITOR_IMPL(Return)
};

#undef VISITOR_IMPL

#define VISITED_NODE_FUNC(nodeName) \
void nodeName##Node::visit(ASTVisitorLink visitor) {\
  visitor->visit##nodeName(this);\
}

VISITED_NODE_FUNC(Block)
VISITED_NODE_FUNC(Expression)
VISITED_NODE_FUNC(Declaration)
VISITED_NODE_FUNC(Branch)
VISITED_NODE_FUNC(Loop)
VISITED_NODE_FUNC(Return)

#undef VISITED_NODE_FUNC

#undef PRETTY_PRINT_FOR
#undef GET_FOR
#undef SET_FOR
#undef GET_SET_FOR

#endif
