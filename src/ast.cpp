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

void NoMoreChildrenNode::addChild(Link child) {
  UNUSED(child);
  throw InternalError("Cannot add children to NoMoreChildrenNode", {METADATA_PAIRS});
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

/**
  \brief Macro to help implement the 'visit' functions in each node
  \param nodeName name of node class, without 'Node' at the end, eg Loop, not LoopNode
*/
#define VISITOR_VISIT_IMPL_FOR(nodeName) \
void nodeName##Node::visit(ASTVisitorLink visitor) {\
  auto shared = this->shared_from_this();\
  visitor->visit##nodeName(Node<nodeName##Node>::staticPtrCast(shared));\
}

VISITOR_VISIT_IMPL_FOR(Block)
VISITOR_VISIT_IMPL_FOR(Expression)
VISITOR_VISIT_IMPL_FOR(Declaration)
VISITOR_VISIT_IMPL_FOR(Branch)
VISITOR_VISIT_IMPL_FOR(Loop)
VISITOR_VISIT_IMPL_FOR(Return)
VISITOR_VISIT_IMPL_FOR(BreakLoop)
VISITOR_VISIT_IMPL_FOR(Function)
VISITOR_VISIT_IMPL_FOR(Type)
VISITOR_VISIT_IMPL_FOR(Constructor)
VISITOR_VISIT_IMPL_FOR(Method)
VISITOR_VISIT_IMPL_FOR(Member)

#undef VISITOR_VISIT_IMPL_FOR


// AST class

AST::AST(Node<BlockNode>::Link lk): root(lk) {}

void AST::print() const {
  ASTPrinter::print(root);
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

// Printer

/// Macro for easy printing of NoMoreChildrenNode children
#define PRETTY_PRINT_FOR(childIndex, nameOf) \
{\
  level++;\
  printIndent(level);\
  println(#nameOf":");\
  node->getChildren()[childIndex]->visit(shared_from_this());\
  level--;\
}

void ASTPrinter::visitBlock(Node<BlockNode>::Link node) {
  printIndent(level);
  println("Block Node:", node->getType());
  printSubtree(node);
}

void ASTPrinter::visitExpression(Node<ExpressionNode>::Link node) {
  printIndent(level);
  println("Expression Node:", node->getToken());
  printSubtree(node);
}

void ASTPrinter::visitDeclaration(Node<DeclarationNode>::Link node) {
  printIndent(level);
  fmt::print("Declaration Node: {0} ({1})\n", node->getIdentifier(), node->getTypeInfo());
  if (node->notNull(0)) printSubtree(node);
}

void ASTPrinter::visitType(Node<TypeNode>::Link node) {
  printIndent(level);
  fmt::print("Type Node: {0} {1}\n", node->getName(),
    node->getAncestors().size() ? "inherits from" + collate(node->getAncestors()) : "\b");
  printSubtree(node);
}

void ASTPrinter::visitConstructor(Node<ConstructorNode>::Link node) {
  printIndent(level);
  fmt::print("Constructor Node: {}\n", node->getSignature());
  if (node->notNull(0)) printSubtree(node);
}

void ASTPrinter::visitMethod(Node<MethodNode>::Link node) {
  printIndent(level);
  fmt::print("Method Node: {0} {1}\n",
    node->getIdentifier(), node->isStatic() ? "(static)" : "");
  printIndent(level);
  println(node->getSignature());
  if (node->notNull(0)) printSubtree(node);
}

void ASTPrinter::visitMember(Node<MemberNode>::Link node) {
  printIndent(level);
  fmt::print("Member Node: {0} ({1}) {2}\n",
    node->getIdentifier(), node->getTypeInfo(), node->isStatic() ? "(static)" : "");
  if (node->notNull(0)) printSubtree(node);
}

void ASTPrinter::visitBranch(Node<BranchNode>::Link node) {
  printIndent(level);
  println("Branch Node:");
  PRETTY_PRINT_FOR(0, Condition)
  PRETTY_PRINT_FOR(1, Success)
  if (node->notNull(2)) PRETTY_PRINT_FOR(2, Failiure)
}

void ASTPrinter::visitLoop(Node<LoopNode>::Link node) {
  printIndent(level);
  println("Loop Node:");
  if (node->notNull(0)) PRETTY_PRINT_FOR(0, Init)
  if (node->notNull(1)) PRETTY_PRINT_FOR(1, Condition)
  if (node->notNull(2)) PRETTY_PRINT_FOR(2, Update)
  if (node->notNull(3)) PRETTY_PRINT_FOR(3, Code)
}

void ASTPrinter::visitReturn(Node<ReturnNode>::Link node) {
  printIndent(level);
  println("Return Node:");
  if (node->notNull(0)) PRETTY_PRINT_FOR(0, Value)
}

void ASTPrinter::visitBreakLoop(Node<BreakLoopNode>::Link node) {
  printIndent(level);
  println("Break Loop:");
  printSubtree(node);
}

void ASTPrinter::visitFunction(Node<FunctionNode>::Link node) {
  printIndent(level);
  fmt::print("Function {0}: {1}\n",
    node->isAnon() ? "<anonymous>" : node->getIdentifier(), node->getSignature());
  if (node->notNull(0)) {
    PRETTY_PRINT_FOR(0, Code)
  } else {
    printIndent(level + 1);
    println("<foreign>");
  }
}

#undef PRETTY_PRINT_FOR

// Equality boilerplate

bool BlockNode::operator==(const ASTNode& rhs) const {
  if (!ASTNode::operator==(rhs)) return false;
  if (this->type != dynamic_cast<const BlockNode&>(rhs).type) return false;
  return true;
}
bool BlockNode::operator!=(const ASTNode& rhs) const {
  return !operator==(rhs);
}

bool ExpressionNode::operator==(const ASTNode& rhs) const {
  if (!ASTNode::operator==(rhs)) return false;
  if (this->tok != dynamic_cast<const ExpressionNode&>(rhs).tok) return false;
  return true;
}
bool ExpressionNode::operator!=(const ASTNode& rhs) const {
  return !operator==(rhs);
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

bool TypeNode::operator==(const ASTNode& rhs) const {
  if (!ASTNode::operator==(rhs)) return false;
  auto decl = dynamic_cast<const TypeNode&>(rhs);
  if (this->name != decl.name) return false;
  if (this->inheritsFrom != decl.inheritsFrom) return false;
  return true;
}
bool TypeNode::operator!=(const ASTNode& rhs) const {
  return !operator==(rhs);
}

bool FunctionNode::operator==(const ASTNode& rhs) const {
  if (!ASTNode::operator==(rhs)) return false;
  auto fun = dynamic_cast<const FunctionNode&>(rhs);
  if (this->ident != fun.ident) return false;
  if (this->sig != fun.sig) return false;
  if (this->foreign != fun.foreign) return false;
  return true;
}
bool FunctionNode::operator!=(const ASTNode& rhs) const {
  return !operator==(rhs);
}

bool ConstructorNode::operator==(const ASTNode& rhs) const {
  if (!FunctionNode::operator==(rhs)) return false;
  auto constr = dynamic_cast<const ConstructorNode&>(rhs);
  if (this->vis != constr.vis) return false;
  return true;
}
bool ConstructorNode::operator!=(const ASTNode& rhs) const {
  return !operator==(rhs);
}

bool MethodNode::operator==(const ASTNode& rhs) const {
  if (!FunctionNode::operator==(rhs)) return false;
  auto meth = dynamic_cast<const MethodNode&>(rhs);
  if (this->vis != meth.vis) return false;
  if (this->staticM != meth.staticM) return false;
  return true;
}
bool MethodNode::operator!=(const ASTNode& rhs) const {
  return !operator==(rhs);
}

bool MemberNode::operator==(const ASTNode& rhs) const {
  if (!DeclarationNode::operator==(rhs)) return false;
  auto mem = dynamic_cast<const MemberNode&>(rhs);
  if (this->vis != mem.vis) return false;
  if (this->staticM != mem.staticM) return false;
  return true;
}
bool MemberNode::operator!=(const ASTNode& rhs) const {
  return !operator==(rhs);
}
