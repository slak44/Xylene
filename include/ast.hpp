#ifndef AST_HPP
#define AST_HPP

#include <string>
#include <vector>
#include <memory>
#include <typeinfo>
#include <unordered_set>

#include "utils/util.hpp"
#include "utils/error.hpp"
#include "utils/typeInfo.hpp"
#include "token.hpp"
#include "llvm/typeId.hpp"

namespace llvm {
  class BasicBlock;
  class Function;
}

class TypeData;
class DeclarationWrapper;
class FunctionWrapper;

class ASTVisitor;
using ASTVisitorLink = PtrUtil<ASTVisitor>::Link;

class ASTNode;
using ASTNodeLink = std::shared_ptr<ASTNode>;

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
  
  /// Technically overrides PtrUtil<T>::dynPtrCast
  static inline Link dynPtrCast(ASTNodeLink node) {
    return std::dynamic_pointer_cast<T>(node);
  }
};

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
  
  static inline void printIndent(unsigned level) noexcept {
    for (unsigned i = 0; i < level; i++) print("  ");
  }
  
  template<typename ReturnType = std::size_t>
  ReturnType transformArrayIndex(int64_t idx) const {
    ReturnType res;
    if (idx < 0) {
      // Negative indices count from the end of the vector
      int64_t transPos = static_cast<int64_t>(children.size()) + idx;
      if (transPos < 0 || transPos > static_cast<int64_t>(children.size())) {
        throw InternalError("Index out of array bounds", {
          METADATA_PAIRS,
          {"index", std::to_string(idx)},
          {"calculated", std::to_string(transPos)}
        });
      }
      res = static_cast<ReturnType>(transPos);
    } else {
      res = static_cast<ReturnType>(idx);
    }
    return res;
  }
  
public:
  ASTNode() = default;
  ASTNode(const ASTNode&) = default;
  virtual ~ASTNode() = default;
  
  /**
    \brief Add a new child to this node.
  */
  virtual void addChild(Link child);
  
  /**
    \brief Remove a child and return it.
    \param pos which child. Supports negative positions that count from the end
  */
  virtual Link removeChild(int64_t pos);
  
  /// Get list of children
  virtual inline Children getChildren() const noexcept {
    return children;
  }
  /**
    \brief Get child at position
    \param pos which child. Supports negative positions that count from the end
  */
  Link at(int64_t pos) const;
  /// \copydoc at(int64)
  Link at(std::size_t pos) const;
  
  inline void setParent(WeakLink newParent) noexcept {
    parent = newParent;
  }
  inline WeakLink getParent() const noexcept {
    return parent;
  }
  
  inline void setTrace(Trace newTrace) noexcept {
    trace = newTrace;
  }
  inline Trace getTrace() const noexcept {
    return trace;
  }
  
  /**
    \brief Traverse the tree upwards until a node satisfies the condition or until we hit the root
    \param isOk must return true for the node that satisfies the condition
    \returns the first node that satisfies isOk, or nullptr if there aren't any
  */
  Link findAbove(std::function<bool(Link)> isOk) const;
  
  /**
    \brief Traverse the tree upwards until a node that matches the type in the template is found
    \returns the first node of type T
  */
  template<typename T>
  typename Node<T>::Link findAbove() const {
    return Node<T>::staticPtrCast(
      findAbove([](ASTNode::Link lk) {
        return Node<T>::dynPtrCast(lk) != nullptr;
      })
    );
  }
  
  /// Pretty printing for this node and all his children
  virtual void printTree(unsigned level) const noexcept;
  
  virtual bool operator==(const ASTNode& rhs) const;
  virtual bool operator!=(const ASTNode& rhs) const;
  
  virtual void visit(ASTVisitorLink visitor) = 0;
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
  /// Maps identifiers to their declaration (contains current value/type)
  std::map<std::string, std::shared_ptr<DeclarationWrapper>> blockScope {};
  /// Maps function names to their respective FunctionWrapper
  std::map<std::string, std::shared_ptr<FunctionWrapper>> blockFuncs {};
  /// List of types defined in this block
  std::unordered_set<AbstractId::Link> blockTypes {};
  
  BlockNode(BlockType type);
  
  inline BlockType getType() const noexcept {
    return type;
  }
  
  void printTree(unsigned level) const noexcept override;
  
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
  Token tok = Token(TT::UNPROCESSED, defaultTrace);
public:
  ExpressionNode(Token token);
  
  /// Technically overrides ASTNode::at
  std::shared_ptr<ExpressionNode> at(int64_t pos) const;
  /// Get stored Token
  inline Token getToken() const noexcept {
    return tok;
  }
  
  void printTree(unsigned level) const noexcept override;
  
  bool operator==(const ASTNode& rhs) const override;
  bool operator!=(const ASTNode& rhs) const override;
  
  void visit(ASTVisitorLink visitor) override;
};

/**
  \brief Definition of a new type.
  
  Contains constructors, methods, and members as children.
*/
class TypeNode: public ASTNode {
private:
  std::string name;
  TypeList inheritsFrom;
  TypeId::Link tid;
public:
  TypeNode(std::string name, TypeList inheritsFrom = {});
  
  /// Get the type name
  inline std::string getName() const noexcept {
    return name;
  }
  
  /// Get who is this inheriting from
  inline TypeList getAncestors() const noexcept {
    return inheritsFrom;
  }

  inline TypeId::Link getTid() const noexcept {
    return tid;
  }

  inline void setTid(TypeId::Link newData) noexcept {
    tid = newData;
  }
  
  /**
    \brief Only accepts ConstructorNode, MethodNode or MemberNode.
    \throws InternalError if other types try to be inserted
  */
  void addChild(Link child) override;
  
  void printTree(unsigned level) const noexcept override;
  
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
GET_SIG(linkType, nameOf) \
SET_SIG(linkType, nameOf)

/**
  \brief ASTNode with fixed children count.
*/
class NoMoreChildrenNode: public ASTNode {
public:
  /**
    \param childrenCount does not have more than this many children
  */
  NoMoreChildrenNode(std::size_t childrenCount) noexcept;
  
  /**
    \throws InternalError always throws, never use
  */
  void addChild(Link child) override;
  
  /// Utility to check if the childIndex'th child is not nullptr
  inline bool notNull(unsigned childIndex) const noexcept {
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
  DeclarationNode(std::string identifier, TypeList typeList) noexcept:
    NoMoreChildrenNode(1), identifier(identifier), info(typeList) {}
    
  GET_SET_SIGS(ExpressionNode, Init)
  
  inline std::string getIdentifier() const noexcept {
    return identifier;
  }

  inline DefiniteTypeInfo getTypeInfo() const noexcept {
    return info;
  }

  inline bool isDynamic() const noexcept {
    return info.isDynamic();
  }

  inline bool hasInit() const noexcept {
    return children[0] != nullptr;
  }
  
  void printTree(unsigned level) const noexcept override;
  
  bool operator==(const ASTNode& rhs) const override;
  bool operator!=(const ASTNode& rhs) const override;
  
  void visit(ASTVisitorLink visitor) override;
};

/**
  \brief If statement in AST form.
  
  Has 3 children:
    - Condition - ExpressionNode
    - SuccessBlock - BlockNode
    - FailiureBlock - BlockNode or BranchNode
*/
class BranchNode: public NoMoreChildrenNode {
public:
  BranchNode() noexcept: NoMoreChildrenNode(3) {}
  
  GET_SET_SIGS(ExpressionNode, Condition)
  GET_SET_SIGS(BlockNode, SuccessBlock)
  GET_SIG(ASTNode, FailiureBlock)
  SET_SIG(BlockNode, FailiureBlock)
  SET_SIG(BranchNode, FailiureBlock)
  
  void printTree(unsigned level) const noexcept override;
  
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
  for Init; Condition; Update Code
*/
class LoopNode: public NoMoreChildrenNode {
private:
  /// Used by the break statement
  llvm::BasicBlock* exitBlock;
public:
  LoopNode() noexcept: NoMoreChildrenNode(4) {}
  
  GET_SET_SIGS(DeclarationNode, Init)
  GET_SET_SIGS(ExpressionNode, Condition)
  GET_SET_SIGS(ExpressionNode, Update)
  GET_SET_SIGS(BlockNode, Code)
  
  inline llvm::BasicBlock* getExitBlock() const noexcept {
    return exitBlock;
  }
  inline void setExitBlock(llvm::BasicBlock* bb) noexcept {
    this->exitBlock = bb;
  }
  
  void printTree(unsigned level) const noexcept override;
  
  void visit(ASTVisitorLink visitor) override;
};

/**
  \brief Return statement.
  
  Can have an ExpressionNode to be returned.
*/
class ReturnNode: public NoMoreChildrenNode {
public:
  ReturnNode() noexcept: NoMoreChildrenNode(1) {}
  
  GET_SET_SIGS(ExpressionNode, Value)
  
  void printTree(unsigned level) const noexcept override;
  
  void visit(ASTVisitorLink visitor) override;
};

/**
  \brief Break statement for exiting a loop.
  
  TODO maybe add loop labels so this can break a specific loop
*/
class BreakLoopNode: public NoMoreChildrenNode {
public:
  BreakLoopNode() noexcept: NoMoreChildrenNode(0) {}
  
  void printTree(unsigned level) const noexcept override;
  
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
  
  inline std::string getIdentifier() const noexcept {
    return ident;
  }
  inline const FunctionSignature& getSignature() const noexcept {
    return sig;
  }
  inline bool isForeign() const noexcept {
    return foreign;
  }
  inline bool isAnon() const noexcept {
    return ident.empty();
  }
  
  void printTree(unsigned level) const noexcept override;
  
  bool operator==(const ASTNode& rhs) const override;
  bool operator!=(const ASTNode& rhs) const override;
  
  void visit(ASTVisitorLink visitor) override;
};

/**
  \brief The visibility specifier for things in a type.
*/
enum Visibility {
  PRIVATE, PROTECTED, PUBLIC,
  INVALID ///< Not actually a valid specifier
};

/**
  \brief Get visibility from token
  
  Defaults to INVALID if token doesn't specify a visibility.
*/
inline Visibility fromToken(Token t) {
  return
    t.type == TT::PRIVATE ? PRIVATE :
    t.type == TT::PROTECT ? PROTECTED :
    t.type == TT::PUBLIC ? PUBLIC :
      INVALID;
}

/**
  \brief Represents a constructor.
  
  It is just a mildly special function.
*/
class ConstructorNode: public FunctionNode {
private:
  Visibility vis;
public:
  ConstructorNode(FunctionSignature::Arguments constructorArgs, Visibility vis = PUBLIC, bool isForeign = false);
  
  inline Visibility getVisibility() const noexcept {
    return vis;
  }
  
  void printTree(unsigned level) const noexcept override;
  
  bool operator==(const ASTNode& rhs) const override;
  bool operator!=(const ASTNode& rhs) const override;
  
  void visit(ASTVisitorLink visitor) override;
};

/**
  \brief Represents a method.
  
  It is just a mildly special function.
*/
class MethodNode: public FunctionNode {
private:
  Visibility vis;
  bool staticM;
public:
  MethodNode(std::string name, FunctionSignature sig, Visibility vis, bool staticM = false, bool isForeign = false);
  
  inline Visibility getVisibility() const noexcept {
    return vis;
  }
    
  inline bool isStatic() const noexcept {
    return staticM;
  }
  
  void printTree(unsigned level) const noexcept override;
  
  bool operator==(const ASTNode& rhs) const override;
  bool operator!=(const ASTNode& rhs) const override;
  
  void visit(ASTVisitorLink visitor) override;
};

/**
  \brief Represents a member declaration.
  
  It is just a mildly special declaration.
*/
class MemberNode: public DeclarationNode {
private:
  bool staticM;
  Visibility vis;
public:
  /// \copydoc DeclarationNode(std::string,TypeList)
  MemberNode(std::string identifier, TypeList typeList, bool staticM = false, Visibility vis = PRIVATE);
  
  inline bool isStatic() const noexcept {
    return staticM;
  }

  inline Visibility getVisibility() const noexcept {
    return vis;
  }
  
  void printTree(unsigned level) const noexcept override;
  
  bool operator==(const ASTNode& rhs) const override;
  bool operator!=(const ASTNode& rhs) const override;
  
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
  ASTVisitor() = default;
  ASTVisitor(const ASTVisitor&) = default;
  virtual ~ASTVisitor() = default;
  PURE_VIRTUAL_VISIT(Block)
  PURE_VIRTUAL_VISIT(Expression)
  PURE_VIRTUAL_VISIT(Declaration)
  PURE_VIRTUAL_VISIT(Branch)
  PURE_VIRTUAL_VISIT(Loop)
  PURE_VIRTUAL_VISIT(Return)
  PURE_VIRTUAL_VISIT(BreakLoop)
  PURE_VIRTUAL_VISIT(Function)
  PURE_VIRTUAL_VISIT(Type)
  PURE_VIRTUAL_VISIT(Constructor)
  PURE_VIRTUAL_VISIT(Method)
  PURE_VIRTUAL_VISIT(Member)
};

#undef PURE_VIRTUAL_VISIT

#endif
