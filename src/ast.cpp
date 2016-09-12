#include "ast.hpp"

ASTNode::ASTNode() {}
ASTNode::~ASTNode() {};

void ASTNode::addChild(Link child) {
  child->setParent(shared_from_this());
  children.push_back(child);
}

ASTNode::Link ASTNode::removeChild(int64 pos) {
  Link child = this->at(pos);
  child->setParent(WeakLink());
  this->children.erase(children.begin() + transformArrayIndex(pos));
  return child;
}

std::size_t ASTNode::transformArrayIndex(int64 idx) const {
  std::size_t res = idx;
  if (idx < 0) {
    res = children.size() + idx; // Negative indices count from the end of the vector
  }
  if (res > children.size()) {
    throw InternalError("Index out of array bounds", {
      METADATA_PAIRS,
      {"index", std::to_string(idx)},
      {"calculated", std::to_string(res)}
    });
  }
  return res;
}

ASTNode::Children ASTNode::getChildren() const {
  return children;
}

ASTNode::Link ASTNode::at(int64 pos) const {
  return children.at(transformArrayIndex(pos));
}

void ASTNode::setParent(WeakLink newParent) {parent = newParent;}
ASTNode::WeakLink ASTNode::getParent() const {return parent;}

void ASTNode::setTrace(Trace trace) {
  this->trace = trace;
}
Trace ASTNode::getTrace() const {
  return trace;
}

bool ASTNode::operator==(const ASTNode& rhs) const {
  if (typeid(*this) != typeid(rhs)) return false;
  if (children.size() != rhs.getChildren().size()) return false;
  for (uint64 i = 0; i < children.size(); i++) {
    if (this->at(i) == nullptr && rhs.at(i) == nullptr) continue;
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

BlockNode::BlockNode(BlockType type): type(type) {}

BlockType BlockNode::getType() const {
  return type;
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

TypeNode::TypeNode(std::string name, TypeList inheritsFrom): name(name), inheritsFrom(inheritsFrom) {}

std::string TypeNode::getName() const {
  return name;
}

TypeList TypeNode::getAncestors() const {
  return inheritsFrom;
}

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

FunctionNode::FunctionNode(std::string ident, FunctionSignature sig, bool foreign):
  NoMoreChildrenNode(1),
  ident(ident),
  sig(sig),
  foreign(foreign) {}
FunctionNode::FunctionNode(FunctionSignature sig): FunctionNode("", sig, false) {}

std::string FunctionNode::getIdentifier() const {
  return ident;
}
const FunctionSignature& FunctionNode::getSignature() const {
  return sig;
}
bool FunctionNode::isForeign() const {
  return foreign;
}
bool FunctionNode::isAnon() const {
  return ident.empty();
}

ConstructorNode::ConstructorNode(FunctionSignature::Arguments args, Visibility vis):
  FunctionNode(FunctionSignature(TypeInfo(nullptr), args)), vis(vis) {}
  
Visibility ConstructorNode::getVisibility() const {
  return vis;
}
  
MethodNode::MethodNode(std::string name, FunctionSignature sig, Visibility vis, bool staticM):
  FunctionNode(name, sig), vis(vis), staticM(staticM) {}
  
Visibility MethodNode::getVisibility() const {
  return vis;
}
  
bool MethodNode::isStatic() const {
  return staticM;
}

MemberNode::MemberNode(std::string identifier, TypeList typeList, bool staticM, Visibility vis):
  DeclarationNode(identifier, typeList), staticM(staticM), vis(vis) {}

bool MemberNode::isStatic() const {
  return staticM;
}

Visibility MemberNode::getVisibility() const {
  return vis;
}

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
