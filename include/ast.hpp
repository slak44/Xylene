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

namespace llvm {
  class BasicBlock;
}

class ASTVisitor;
using ASTVisitorLink = PtrUtil<ASTVisitor>::Link;

/**
  \brief A node in an AST
  
  This is the abstract superclass for all the nodes in an AST.
*/
class ASTNode: public std::enable_shared_from_this<ASTNode> {
public:
  using Link = std::shared_ptr<ASTNode>;
  using WeakLink = std::weak_ptr<ASTNode>;
  using Children = std::vector<Link>;
protected:
  WeakLink parent = WeakLink(); ///< Parent in the tree
  Children children {}; ///< Branches/Leaves in the tree
  Trace trace = defaultTrace; ///< Where in the source was this found
  
  static inline void printIndent(uint level) {
    for (uint i = 0; i < level; i++) print("  ");
  }
  
  std::size_t transformArrayIndex(int64 idx) const;
public:
  ASTNode();
  virtual ~ASTNode();
  
  /**
    \brief Add a new child to this node.
  */
  virtual void addChild(Link child);
  
  /**
    \brief Remove a child and return it.
    \param pos which child. Suports negative positions that count from the end of the children
  */
  virtual Link removeChild(int64 pos);
  
  /// Get list of children
  Children getChildren() const;
  /**
    \brief Get child at position
    \param pos which child. Suports negative positions that count from the end of the children
  */
  Link at(int64 pos) const;
  
  void setParent(WeakLink newParent);
  WeakLink getParent() const;
  
  void setTrace(Trace trace);
  Trace getTrace() const;
  
  /// Pretty printing for this node and all his children
  virtual void printTree(uint level) const;
  
  virtual bool operator==(const ASTNode& rhs) const;
  virtual bool operator!=(const ASTNode& rhs) const;
  
  virtual void visit(ASTVisitorLink visitor) = 0;
};

/**
  \brief Utility class for managing smart pointers of ASTNode subclasses
*/
template<typename T, typename std::enable_if<std::is_base_of<ASTNode, T>::value>::type* = nullptr>
struct Node: public PtrUtil<T> {
  typedef typename PtrUtil<T>::Link Link;
  typedef typename PtrUtil<T>::WeakLink WeakLink;
  
  /// Create a shared ptr that holds a node
  template<typename... Args>
  static Link make(Args... args) {
    return std::make_shared<T>(args...);
  }
  
  /// Technically overrides PtrUtil<T>::isSameType
  static inline bool isSameType(ASTNode::Link node) {
    return typeid(T) == typeid(*node);
  }
  
  /// Technically overrides PtrUtil<T>::dynPtrCast
  static inline Link dynPtrCast(ASTNode::Link node) {
    return std::dynamic_pointer_cast<T>(node);
  }
};

/**
  \brief All the types a BlockNode can be.
*/
enum BlockType {
  ROOT_BLOCK, ///< Only the root of the AST is this.
  CODE_BLOCK, ///< Default. Normal block do...end
  FUNCTION_BLOCK, ///< Same as CODE_BLOCK, except it is only used for functions
  IF_BLOCK ///< Used for else clauses. Can be do...else
};

/**
  \brief Represents a block of statements. Children are those statements.
*/
class BlockNode: public ASTNode {
private:
  BlockType type;
public:
  BlockNode(BlockType type);
  
  BlockType getType() const;
  
  void printTree(uint level) const override;
  
  bool operator==(const ASTNode& rhs) const override;
  bool operator!=(const ASTNode& rhs) const override;
  
  void visit(ASTVisitorLink visitor) override;
};

/**
  \brief Represents a piece of an expression.
  
  The stored token only contains operators and terminals.
  If it has an operator, the children are the operands.
  If it has a terminal, it does not have children.
*/
class ExpressionNode: public ASTNode {
private:
  Token tok = Token(UNPROCESSED, defaultTrace);
public:
  ExpressionNode(Token token);
  
  /// Technically overrides ASTNode::at
  std::shared_ptr<ExpressionNode> at(int64 pos) const;
  /// Get stored Token
  Token getToken() const;
  
  void printTree(uint level) const override;
  
  bool operator==(const ASTNode& rhs) const override;
  bool operator!=(const ASTNode& rhs) const override;
  
  void visit(ASTVisitorLink visitor) override;
};

/// \see GET_FOR
#define GET_SIG(linkType, nameOf) Node<linkType>::Link get##nameOf() const;
/// \see SET_FOR
#define SET_SIG(linkType, nameOf) void set##nameOf(std::shared_ptr<linkType> newNode);
/// \see GET_SET_FOR
#define GET_SET_SIGS(linkType, nameOf) \
GET_SIG(linkType, nameOf); \
SET_SIG(linkType, nameOf);

/**
  \brief ASTNode with fixed children count.
*/
class NoMoreChildrenNode: public ASTNode {
public:
  /**
    \param childrenCount does not have more than this many children
  */
  NoMoreChildrenNode(int childrenCount);
  
  /**
    \throws InternalError always throws, never use
  */
  void addChild(Link child) override;
  
  /// Utility to check if the childIndex'th child is not nullptr
  bool inline notNull(uint childIndex) const {
    return children[childIndex] != nullptr;
  }
};

/**
  \brief Stores metadata about a declaration.
  
  Can have one ExpressionNode child representing initialization. Check using hasInit.
*/
class DeclarationNode: public NoMoreChildrenNode {
private:
  std::string identifier;
  DefiniteTypeInfo info;
public:
  /**
    \param identifier name of declared variable
    \param typeList what types are allowed to be stored in this variable
  */
  DeclarationNode(std::string identifier, TypeList typeList);
    
  GET_SET_SIGS(ExpressionNode, Init)
  
  /// Get string identifier
  std::string getIdentifier() const;
  /// Get DefiniteTypeInfo
  DefiniteTypeInfo getTypeInfo() const;
  
  /// Has dynamic type
  bool isDynamic() const;
  /// Has initialization
  bool hasInit() const;
  
  void printTree(uint level) const override;
  
  bool operator==(const ASTNode& rhs) const override;
  bool operator!=(const ASTNode& rhs) const override;
  
  void visit(ASTVisitorLink visitor) override;
};

/**
  \brief If statement in AST form.
  
  Has 3 children:
    - Condition - ExpressionNode
    - SuccessBlock - BlockNode - if Condition is true, run this
    - FailiureBlock - BlockNode or BranchNode - if Condition if false, run this
*/
class BranchNode: public NoMoreChildrenNode {
public:
  BranchNode();
  
  GET_SET_SIGS(ExpressionNode, Condition)
  GET_SET_SIGS(BlockNode, SuccessBlock)
  GET_SIG(ASTNode, FailiureBlock)
  SET_SIG(BlockNode, FailiureBlock)
  SET_SIG(BranchNode, FailiureBlock)
  
  void printTree(uint level) const override;
  
  void visit(ASTVisitorLink visitor) override;
};

/**
  \brief Represents a loop.
  
  Has 4 children:
    - Init - DeclarationNode
    - Condition - ExpressionNode
    - Update - ExpressionNode
    - Code - BlockNode
  Example on a for loop:
  for (Init, Condition, Update) Code
*/
class LoopNode: public NoMoreChildrenNode {
private:
  /// Used by the break statement
  llvm::BasicBlock* exitBlock;
public:
  LoopNode();
  
  GET_SET_SIGS(DeclarationNode, Init)
  GET_SET_SIGS(ExpressionNode, Condition)
  GET_SET_SIGS(ExpressionNode, Update)
  GET_SET_SIGS(BlockNode, Code)
  
  llvm::BasicBlock* getExitBlock() const;
  void setExitBlock(llvm::BasicBlock* bb);
  
  void printTree(uint level) const override;
  
  void visit(ASTVisitorLink visitor) override;
};

/**
  \brief Return statement.
  
  Can have an ExpressionNode to be returned.
*/
class ReturnNode: public NoMoreChildrenNode {
public:
  ReturnNode();
  
  GET_SET_SIGS(ExpressionNode, Value)
  
  void printTree(uint level) const override;
  
  void visit(ASTVisitorLink visitor) override;
};

/**
  \brief Break statement for exiting a loop.
  
  \todo maybe add loop labels so this can break a specific loop
*/
class BreakLoopNode: public NoMoreChildrenNode {
public:
  BreakLoopNode();
  
  void printTree(uint level) const override;
  
  void visit(ASTVisitorLink visitor) override;
};

/**
  \brief Function definition.
*/
class FunctionNode: public NoMoreChildrenNode {
private:
  std::string ident;
  FunctionSignature sig;
  bool foreign;
public:
  FunctionNode(FunctionSignature sig);
  /// \param ident if empty, behaves just like FunctionNode(FunctionSignature)
  FunctionNode(std::string ident, FunctionSignature sig, bool foreign = false);
  
  GET_SET_SIGS(BlockNode, Code)
  
  std::string getIdentifier() const;
  const FunctionSignature& getSignature() const;
  bool isForeign() const;
  bool isAnon() const;
  
  void printTree(uint level) const override;
  
  void visit(ASTVisitorLink visitor) override;
};

#undef GET_SIG
#undef SET_SIG
#undef GET_SET_SIGS

/**
  \brief Stores an abstract syntax tree of a program.
*/
class AST {
private:
  /// Root of AST
  Node<BlockNode>::Link root;
public:
  AST(Node<BlockNode>::Link lk);
  /// Print the tree
  void print() const;
  /// Get root
  Node<BlockNode>::Link getRoot() const;
  
  bool operator==(const AST& rhs) const;
  bool operator!=(const AST& rhs) const;
};

/**
  \see VISITOR_VISIT_IMPL_FOR
*/
#define PURE_VIRTUAL_VISIT(nodeName) virtual void visit##nodeName(Node<nodeName##Node>::Link node) = 0;

/**
  \brief Abstract class for visiting nodes on an AST.
*/
class ASTVisitor {
public:
  PURE_VIRTUAL_VISIT(Block)
  PURE_VIRTUAL_VISIT(Expression)
  PURE_VIRTUAL_VISIT(Declaration)
  PURE_VIRTUAL_VISIT(Branch)
  PURE_VIRTUAL_VISIT(Loop)
  PURE_VIRTUAL_VISIT(Return)
  PURE_VIRTUAL_VISIT(BreakLoop)
  PURE_VIRTUAL_VISIT(Function)
};

#undef PURE_VIRTUAL_VISIT

#endif
