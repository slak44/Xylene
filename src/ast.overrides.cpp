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

void FunctionNode::printTree(uint level) const {
  printIndent(level);
  println("Function", isAnon() ? "<anonymous>" : ident);
  printIndent(level);
  println(this->sig.toString());
  if (notNull(0)) getCode()->printTree(level + 1);
}

#undef PRETTY_PRINT_FOR
