#include "ast.hpp"

void ASTNode::addChild(Link child) {
  child->setParent(shared_from_this());
  children.push_back(child);
}

ASTNode::Link ASTNode::removeChild(int64_t pos) {
  Link child = this->at(pos);
  child->setParent(WeakLink());
  this->children.erase(std::next(std::begin(children), transformArrayIndex<int64_t>(pos)));
  return child;
}

ASTNode::Link ASTNode::at(int64_t pos) const {
  return children.at(transformArrayIndex(pos));
}

ASTNode::Link ASTNode::at(std::size_t pos) const {
  return children.at(pos);
}

ASTNode::Link ASTNode::findAbove(std::function<bool(Link)> isOk) const {
  auto lastParent = this->getParent();
  for (; ; lastParent = lastParent.lock()->getParent()) {
    if (lastParent.lock() == nullptr) return nullptr;
    if (isOk(lastParent.lock())) return lastParent.lock();
  }
}

bool ASTNode::operator==(const ASTNode& rhs) const {
  if (typeid(*this) != typeid(rhs)) return false;
  if (children.size() != rhs.getChildren().size()) return false;
  for (std::size_t i = 0; i < children.size(); i++) {
    if (this->at(i) == nullptr && rhs.at(i) == nullptr) continue;
    if (*(this->at(i)) != *(rhs.at(i))) return false;
  }
  return true;
}

bool ASTNode::operator!=(const ASTNode& rhs) const {
  return !operator==(rhs);
}

NoMoreChildrenNode::NoMoreChildrenNode(std::size_t childrenCount) noexcept {
  children.resize(childrenCount, nullptr);
}

BlockNode::BlockNode(BlockType type): type(type) {}

ExpressionNode::ExpressionNode(Token token): tok(token) {
  switch (int(tok.type)) {
    case TT::IDENTIFIER:
    case TT::OPERATOR:
    case TT::INTEGER:
    case TT::FLOAT:
    case TT::STRING:
    case TT::BOOLEAN:
      break;
    default: throw InternalError("Trying to add unsupported token to ExpressionNode", {METADATA_PAIRS, {"token", token.toString()}});
  }
}

std::shared_ptr<ExpressionNode> ExpressionNode::at(int64_t pos) const {
  return std::dynamic_pointer_cast<ExpressionNode>(ASTNode::at(pos));
}

TypeNode::TypeNode(std::string name, TypeList inheritsFrom):
  name(name),
  inheritsFrom(inheritsFrom) {}

void TypeNode::addChild(Link child) {
  if (
    Node<ConstructorNode>::dynPtrCast(child) ||
    Node<MethodNode>::dynPtrCast(child) ||
    Node<MemberNode>::dynPtrCast(child)
  ) {
    ASTNode::addChild(child);
  } else {
    throw InternalError("Attempt to add invalid type to TypeNode", {
      METADATA_PAIRS,
      {"valid types", "ConstructorNode MethodNode MemberNode"}
    });
  }
}

FunctionNode::FunctionNode(std::string ident, FunctionSignature sig, bool foreign):
  NoMoreChildrenNode(1),
  ident(ident),
  sig(sig),
  foreign(foreign) {}
FunctionNode::FunctionNode(FunctionSignature sig): FunctionNode("", sig, false) {}

ConstructorNode::ConstructorNode(FunctionSignature::Arguments args, Visibility vis, bool isForeign):
  FunctionNode("constructor", FunctionSignature(nullptr, args), isForeign), vis(vis) {
  if (vis == INVALID) throw InternalError("Invalid visibility", {METADATA_PAIRS});
}
  
MethodNode::MethodNode(std::string name, FunctionSignature sig, Visibility vis, bool staticM, bool isForeign):
  FunctionNode(name, sig, isForeign), vis(vis), staticM(staticM) {
  if (vis == INVALID) throw InternalError("Invalid visibility", {METADATA_PAIRS});
}
  
MemberNode::MemberNode(std::string identifier, TypeList typeList, bool staticM, Visibility vis):
  DeclarationNode(identifier, typeList), staticM(staticM), vis(vis) {}

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
  return Node<linkType>::staticPtrCast(children[childIndex]);\
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

GET_SET_FOR(FunctionNode, 0, Code, BlockNode)

#undef GET_FOR
#undef SET_FOR
#undef GET_SET_FOR

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
