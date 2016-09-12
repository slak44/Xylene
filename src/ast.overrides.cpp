#include "ast.hpp"

void NoMoreChildrenNode::addChild(Link child) {
  UNUSED(child);
  throw InternalError("Cannot add children to NoMoreChildrenNode", {METADATA_PAIRS});
}

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

bool MethodNode::operator==(const ASTNode& rhs) const {
  if (!FunctionNode::operator==(rhs)) return false;
  auto meth = dynamic_cast<const MethodNode&>(rhs);
  if (this->staticM != meth.staticM) return false;
  return true;
}
bool MethodNode::operator!=(const ASTNode& rhs) const {
  return !operator==(rhs);
}

bool MemberNode::operator==(const ASTNode& rhs) const {
  if (!DeclarationNode::operator==(rhs)) return false;
  auto mem = dynamic_cast<const MemberNode&>(rhs);
  if (this->staticM != mem.staticM) return false;
  return true;
}
bool MemberNode::operator!=(const ASTNode& rhs) const {
  return !operator==(rhs);
}

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
VISITOR_VISIT_IMPL_FOR(Function);
VISITOR_VISIT_IMPL_FOR(Type);
VISITOR_VISIT_IMPL_FOR(Constructor);
VISITOR_VISIT_IMPL_FOR(Method);
VISITOR_VISIT_IMPL_FOR(Member);

#undef VISITOR_VISIT_IMPL_FOR

/// Macro for easy printing of NoMoreChildrenNode children
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
  if (notNull(0)) children[0]->printTree(level + 1);
}

void TypeNode::printTree(uint level) const {
  printIndent(level);
  println("Type Node " + name + " inherits from " + collate(inheritsFrom));
  for (auto& child : children) child->printTree(level + 1);
}

void ConstructorNode::printTree(uint level) const {
  printIndent(level);
  println("ConstructorNode " + getSignature().toString());
  if (notNull(0)) getCode()->printTree(level + 1);
}

void MethodNode::printTree(uint level) const {
  printIndent(level);
  println("MethodNode " + getIdentifier() + (isStatic() ? "(static)" : ""));
  printIndent(level);
  println(getSignature().toString());
  if (notNull(0)) getCode()->printTree(level + 1);
}

void MemberNode::printTree(uint level) const {
  printIndent(level);
  println("Member Node: " + getIdentifier() + (isStatic() ? "(static)" : "") + " (" + getTypeInfo().toString() + ")");
  if (notNull(0)) children[0]->printTree(level + 1);
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

void FunctionNode::printTree(uint level) const {
  printIndent(level);
  println("Function", isAnon() ? "<anonymous>" : ident);
  printIndent(level);
  println(this->sig.toString());
  if (notNull(0)) {
    getCode()->printTree(level + 1);
  } else {
    printIndent(level + 1);
    println("<foreign>");
  }
}

#undef PRETTY_PRINT_FOR
