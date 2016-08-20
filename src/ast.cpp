#include "ast.hpp"

ASTNode::ASTNode(uint64 lineNumber): lineNumber(lineNumber) {}
ASTNode::~ASTNode() {};

void ASTNode::addChild(Link child) {
  child->setParent(shared_from_this());
  children.push_back(child);
}

ASTNode::Children ASTNode::getChildren() const {
  return children;
}

ASTNode::Link ASTNode::at(int64 pos) const {
  if (pos < 0) {
    pos = children.size() + pos; // Negative indices count from the end of the vector
  }
  if (static_cast<std::size_t>(abs(pos)) > children.size() || pos < 0) {
    throw InternalError("Index out of array bounds", {METADATA_PAIRS, {"index", std::to_string(pos)}});
  }
  return children[pos];
}

void ASTNode::setParent(WeakLink newParent) {parent = newParent;}
ASTNode::WeakLink ASTNode::getParent() const {return parent;}

void ASTNode::setLineNumber(uint64 newLineNumber) {
  lineNumber = newLineNumber;
}
uint64 ASTNode::getLineNumber() const {
  return lineNumber;
}

bool ASTNode::operator==(const ASTNode& rhs) const {
  if (typeid(*this) != typeid(rhs)) return false;
  if (children.size() != rhs.getChildren().size()) return false;
  for (uint64 i = 0; i < children.size(); i++) {
    if (*(this->at(i)) != *(rhs.at(i))) return false;
  }
  return true;
}

bool ASTNode::operator!=(const ASTNode& rhs) const {
  return !operator==(rhs);
}

NoMoreChildrenNode::NoMoreChildrenNode(int childrenCount) {
  children.resize(childrenCount, nullptr);
}

void NoMoreChildrenNode::addChild(Link child) {
  UNUSED(child);
  throw InternalError("Cannot add children to NoMoreChildrenNode", {METADATA_PAIRS});
}

BlockNode::BlockNode(BlockType type): type(type) {}

BlockType BlockNode::getType() const {
  return type;
}

bool BlockNode::operator==(const ASTNode& rhs) const {
  if (!ASTNode::operator==(rhs)) return false;
  if (this->type != dynamic_cast<const BlockNode&>(rhs).type) return false;
  return true;
}
bool BlockNode::operator!=(const ASTNode& rhs) const {
  return !operator==(rhs);
}

ExpressionNode::ExpressionNode(Token token): tok(token) {
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

std::shared_ptr<ExpressionNode> ExpressionNode::at(int64 pos) const {
  return std::dynamic_pointer_cast<ExpressionNode>(ASTNode::at(pos));
}

Token ExpressionNode::getToken() const {
  return tok;
}

bool ExpressionNode::operator==(const ASTNode& rhs) const {
  if (!ASTNode::operator==(rhs)) return false;
  if (this->tok != dynamic_cast<const ExpressionNode&>(rhs).tok) return false;
  return true;
}
bool ExpressionNode::operator!=(const ASTNode& rhs) const {
  return !operator==(rhs);
}

DeclarationNode::DeclarationNode(std::string identifier, TypeList typeList):
  NoMoreChildrenNode(1), identifier(identifier), info(typeList) {}
  
std::string DeclarationNode::getIdentifier() const {
  return identifier;
}

DefiniteTypeInfo DeclarationNode::getTypeInfo() const {
  return info;
}

bool DeclarationNode::isDynamic() const {
  return info.isDynamic();
}

bool DeclarationNode::hasInit() const {
  return children[0] != nullptr;
}

bool DeclarationNode::operator==(const ASTNode& rhs) const {
  if (!ASTNode::operator==(rhs)) return false;
  auto decl = dynamic_cast<const DeclarationNode&>(rhs);
  if (this->identifier != decl.identifier) return false;
  if (this->info != decl.info) return false;
  return true;
}
bool DeclarationNode::operator!=(const ASTNode& rhs) const {
  return !operator==(rhs);
}

BranchNode::BranchNode(): NoMoreChildrenNode(3) {}

LoopNode::LoopNode(): NoMoreChildrenNode(4) {}

llvm::BasicBlock* LoopNode::getExitBlock() const {
  return exitBlock;
}
void LoopNode::setExitBlock(llvm::BasicBlock* bb) {
  this->exitBlock = bb;
}

ReturnNode::ReturnNode(): NoMoreChildrenNode(1) {}

BreakLoopNode::BreakLoopNode(): NoMoreChildrenNode(0) {}

// Macro for easy printing of NoMoreChildrenNode children
#define PRETTY_PRINT_FOR(childIndex, nameOf) \
{\
  printIndent(level + 1);\
  println(#nameOf":");\
  children[childIndex]->printTree(level + 1);\
}

void ASTNode::printTree(uint level) const {
  printIndent(level);
  println("Base ASTNode");
  for (auto& child : children) child->printTree(level + 1);
}

void BlockNode::printTree(uint level) const {
  printIndent(level);
  println("Block Node:", type);
  for (auto& child : children) child->printTree(level + 1);
}

void ExpressionNode::printTree(uint level) const {
  printIndent(level);
  println("Expression Node:", tok);
  for (auto& child : children) child->printTree(level + 1);
}

void DeclarationNode::printTree(uint level) const {
  printIndent(level);
  println("Declaration Node: " + identifier + " (" + info.toString() + ")");
  if (children.size() > 0) children[0]->printTree(level + 1);
}

void BranchNode::printTree(uint level) const {
  printIndent(level);
  println("Branch Node:");
  PRETTY_PRINT_FOR(0, Condition)
  PRETTY_PRINT_FOR(1, Success)
  if (notNull(2)) PRETTY_PRINT_FOR(2, Failiure)
}

void LoopNode::printTree(uint level) const {
  printIndent(level);
  println("Loop Node");
  if (notNull(0)) PRETTY_PRINT_FOR(0, Init)
  if (notNull(1)) PRETTY_PRINT_FOR(1, Condition)
  if (notNull(2)) PRETTY_PRINT_FOR(2, Update)
  if (notNull(3)) PRETTY_PRINT_FOR(3, Code)
}

void ReturnNode::printTree(uint level) const {
  printIndent(level);
  println("Return Node");
  if (notNull(0)) PRETTY_PRINT_FOR(0, Value)
}

void BreakLoopNode::printTree(uint level) const {
  printIndent(level);
  println("Break Loop");
  for (auto& child : children) child->printTree(level + 1);
}

#undef PRETTY_PRINT_FOR

/**
  \brief Macros for easy implementation of getters and setters for NoMoreChildrenNode subclasses
  
  Omologues exist in header to provide signatures for these implementations
  \param srcNode name of subclass
  \param childIndex index for the child (because number of children is fixed on NoMoreChildrenNodes)
  \param nameOf name for getter/setter (get##nameOf and set##nameOf)
  \param linkType type of child
*/
#define GET_FOR(srcNode, childIndex, nameOf, linkType) \
Node<linkType>::Link srcNode::get##nameOf() const {\
  return Node<linkType>::dynPtrCast(children[childIndex]);\
}
/// \copydoc GET_FOR
#define SET_FOR(srcNode, childIndex, nameOf, linkType) \
void srcNode::set##nameOf(std::shared_ptr<linkType> newNode) {\
  newNode->setParent(shared_from_this());\
  children[childIndex] = newNode;\
}
/// \copydoc GET_FOR
#define GET_SET_FOR(srcNode, childIndex, nameOf, linkType) \
GET_FOR(srcNode, childIndex, nameOf, linkType) \
SET_FOR(srcNode, childIndex, nameOf, linkType)

GET_SET_FOR(DeclarationNode, 0, Init, ExpressionNode)

GET_SET_FOR(BranchNode, 0, Condition, ExpressionNode)
GET_SET_FOR(BranchNode, 1, SuccessBlock, BlockNode)
GET_FOR(BranchNode, 2, FailiureBlock, ASTNode)
SET_FOR(BranchNode, 2, FailiureBlock, BlockNode)
SET_FOR(BranchNode, 2, FailiureBlock, BranchNode)

GET_SET_FOR(LoopNode, 0, Init, DeclarationNode)
GET_SET_FOR(LoopNode, 1, Condition, ExpressionNode)
GET_SET_FOR(LoopNode, 2, Update, ExpressionNode)
GET_SET_FOR(LoopNode, 3, Code, BlockNode)

GET_SET_FOR(ReturnNode, 0, Value, ExpressionNode)

#undef GET_FOR
#undef SET_FOR
#undef GET_SET_FOR

/**
  \brief Macro to help implement the 'visit' functions in each node
  \param nodeName name of node class, without 'Node' at the end, eg Loop, not LoopNode
*/
#define VISITOR_VISIT_IMPL_FOR(nodeName) \
void nodeName##Node::visit(ASTVisitorLink visitor) {\
  auto shared = this->shared_from_this();\
  visitor->visit##nodeName(Node<nodeName##Node>::staticPtrCast(shared));\
}

VISITOR_VISIT_IMPL_FOR(Block);
VISITOR_VISIT_IMPL_FOR(Expression);
VISITOR_VISIT_IMPL_FOR(Declaration);
VISITOR_VISIT_IMPL_FOR(Branch);
VISITOR_VISIT_IMPL_FOR(Loop);
VISITOR_VISIT_IMPL_FOR(Return);
VISITOR_VISIT_IMPL_FOR(BreakLoop);

#undef VISITOR_VISIT_IMPL_FOR

// AST class

AST::AST(Node<BlockNode>::Link lk): root(lk) {}

void AST::print() const {
  root->printTree(0);
}

Node<BlockNode>::Link AST::getRoot() const {
  return root;
}

bool AST::operator==(const AST& rhs) const {
  if (*this->root.get() != *rhs.root.get()) return false;
  return true;
}
bool AST::operator!=(const AST& rhs) const {
  return !operator==(rhs);
}
