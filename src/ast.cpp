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
  return getChildren().at(transformArrayIndex(pos));
}

ASTNode::Link ASTNode::at(std::size_t pos) const {
  return getChildren().at(pos);
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
  if (getChildren().size() != rhs.getChildren().size()) return false;
  for (std::size_t i = 0; i < getChildren().size(); i++) {
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
  println(ASTPrinter::print(root));
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
#define PRETTY_PRINT_FOR(child, nameOf) \
level++;\
printIndent();\
println(nameOf":");\
child->visit(shared_from_this());\
level--;

void ASTPrinter::visitBlock(Node<BlockNode>::Link node) {
  printIndent();
  println(fmt::format("Block Node: {}", node->getType()));
  printSubtree(node);
}

void ASTPrinter::visitExpression(Node<ExpressionNode>::Link node) {
  printIndent();
  println(fmt::format("Expression Node: {}", node->getToken()));
  printSubtree(node);
}

void ASTPrinter::visitDeclaration(Node<DeclarationNode>::Link node) {
  printIndent();
  println(fmt::format("Declaration Node: {0} ({1})", node->getIdentifier(), node->getTypeInfo()));
  if (node->notNull(0)) printSubtree(node);
}

void ASTPrinter::visitType(Node<TypeNode>::Link node) {
  printIndent();
  println(fmt::format("Type Node: {0} {1}", node->getName(),
    node->getAncestors().size() ? "inherits from" + collate(node->getAncestors()) : "\b"));
  printSubtree(node);
}

void ASTPrinter::visitConstructor(Node<ConstructorNode>::Link node) {
  printIndent();
  println(fmt::format("Constructor Node: {}", node->getSignature()));
  if (node->notNull(0)) printSubtree(node);
}

void ASTPrinter::visitMethod(Node<MethodNode>::Link node) {
  printIndent();
  println(fmt::format("Method Node: {0} {1}",
    node->getIdentifier(), node->isStatic() ? "(static)" : ""));
  printIndent();
  println(node->getSignature().toString());
  if (node->notNull(0)) printSubtree(node);
}

void ASTPrinter::visitMember(Node<MemberNode>::Link node) {
  printIndent();
  println(fmt::format("Member Node: {0} ({1}) {2}",
    node->getIdentifier(), node->getTypeInfo(), node->isStatic() ? "(static)" : ""));
  if (node->notNull(0)) printSubtree(node);
}

void ASTPrinter::visitBranch(Node<BranchNode>::Link node) {
  printIndent();
  println("Branch Node:");
  PRETTY_PRINT_FOR(node->condition(), "Condition");
  PRETTY_PRINT_FOR(node->success(), "Success");
  PRETTY_PRINT_FOR(node->getChildren()[2], "Failiure");
}

void ASTPrinter::visitLoop(Node<LoopNode>::Link node) {
  printIndent();
  println("Loop Node:");
  for (auto init : node->inits()) {
    PRETTY_PRINT_FOR(init, "Init");
  }
  if (node->condition() != nullptr) {
    PRETTY_PRINT_FOR(node->condition(), "Condition");
  }
  for (auto upd : node->updates()) {
    PRETTY_PRINT_FOR(upd, "Update");
  }
  if (node->code() != nullptr) {
    PRETTY_PRINT_FOR(node->code(), "Code");
  }
}

void ASTPrinter::visitReturn(Node<ReturnNode>::Link node) {
  printIndent();
  println("Return Node:");
  if (node->value() != nullptr) PRETTY_PRINT_FOR(node->value(), "Value");
}

void ASTPrinter::visitBreakLoop(Node<BreakLoopNode>::Link node) {
  printIndent();
  println("Break Loop:");
  printSubtree(node);
}

void ASTPrinter::visitFunction(Node<FunctionNode>::Link node) {
  printIndent();
  println(fmt::format("Function {0}: {1}",
    node->isAnon() ? "<anonymous>" : node->getIdentifier(), node->getSignature()));
  if (node->code() != nullptr) {
    PRETTY_PRINT_FOR(node->code(), "Code");
  } else {
    level++;
    printIndent();
    println("<foreign>");
    level--;
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
