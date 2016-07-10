#ifndef AST_HPP
#define AST_HPP

#include <string>
#include <sstream>
#include <vector>
#include <set>
#include <memory>
#include <numeric>
#include <typeinfo>
#include <rapidxml.hpp>

#include "error.hpp"
#include "token.hpp"
#include "util.hpp"
#include "operator.hpp"

typedef std::set<std::string> TypeList;

class ASTNode: public std::enable_shared_from_this<ASTNode> {
public:
  typedef std::shared_ptr<ASTNode> Link;
  typedef std::weak_ptr<ASTNode> WeakLink;
  typedef std::vector<Link> Children;
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
    if (abs(pos) > children.size() || pos < 0) {
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
};

#define GET_SET_FOR(childIndex, nameOf, linkType) \
void set##nameOf(Node<linkType>::Link newNode) {children[childIndex] = newNode;}\
Node<linkType>::Link get##nameOf() const {\
  return Node<linkType>::dynPtrCast(children[childIndex]);\
}

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
};

class DeclarationNode: public ASTNode {
private:
  std::string identifier;
  TypeList typeList;
  bool dynamic;
public:
  DeclarationNode(std::string identifier, TypeList typeList):
    identifier(identifier), typeList(typeList), dynamic(false) {}
  DeclarationNode(std::string identifier):
    identifier(identifier), typeList({}), dynamic(true) {}
    
  std::string getIdentifier() const {
    return identifier;
  }
  
  TypeList getTypeList() const {
    return typeList;
  }
  
  bool isDynamic() const {
    return dynamic;
  }
  
  bool hasInit() const {
    return children.size() == 1;
  }
  
  Node<ExpressionNode>::Link getInit() const {
    return Node<ExpressionNode>::dynPtrCast(children[0]);
  }
  
  void addChild(Link child) {
    if (children.size() >= 1) throw InternalError("Trying to add more than one child to a DeclarationNode", {METADATA_PAIRS});
    if (!Node<ExpressionNode>::dynPtrCast(child)) throw InternalError("DeclarationNode only supports ExpressionNode as its child", {METADATA_PAIRS});
    ASTNode::addChild(child);
  }
  
  void printTree(uint level) const {
    printIndent(level);
    std::string collatedTypes = "";
    if (dynamic) collatedTypes = "[dynamic]";
    if (!dynamic && typeList.size() == 0) throw InternalError("No types in typed declaration", {METADATA_PAIRS});
    if (typeList.size() == 1) {
      collatedTypes = *typeList.begin();
    }
    if (typeList.size() >= 2) {
      collatedTypes = std::accumulate(++typeList.begin(), typeList.end(), *typeList.begin(),
      [](const std::string& prev, const std::string& current) {
        return prev + ", " + current;
      });
    }
    println("Declaration Node: " + identifier + " (valid types: " + collatedTypes + ")");
    if (children.size() > 0) children[0]->printTree(level + 1);
  }
  
  bool operator==(const ASTNode& rhs) const {
    if (!ASTNode::operator==(rhs)) return false;
    auto decl = dynamic_cast<const DeclarationNode&>(rhs);
    if (this->identifier != decl.identifier) return false;
    if (this->typeList != decl.typeList) return false;
    if (this->dynamic != decl.dynamic) return false;
    return true;
  }
  bool operator!=(const ASTNode& rhs) const {
    return !operator==(rhs);
  }
};

class BranchNode: public NoMoreChildrenNode {
public:
  BranchNode(): NoMoreChildrenNode(3) {}
  
  GET_SET_FOR(0, Condition, ExpressionNode)
  GET_SET_FOR(1, SuccessBlock, BlockNode)
  GET_SET_FOR(2, FailiureBlock, ASTNode)
  
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

#undef PRETTY_PRINT_FOR
#undef GET_SET_FOR

#endif
