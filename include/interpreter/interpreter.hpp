#ifndef INTERPRETER_HPP
#define INTERPRETER_HPP

#include <string>
#include <vector>

#include "ast.hpp"
#include "utils/error.hpp"
#include "utils/util.hpp"
#include "object.hpp"
#include "token.hpp"
#include "operatorMap.hpp"
#include "scope.hpp"

/**
  \brief Interpret an AST by walking the tree.
  \deprecated Superseded by the LLVM compiler+interpreter
*/
class TreeWalkInterpreter {
private:
  void interpretBlock(Scope::Link parentScope, Node<BlockNode>::Link block) {
    Scope::Link thisBlockScope = std::make_shared<Scope>();
    thisBlockScope->setParent(parentScope);
    for (auto& child : block->getChildren()) {
      interpretStatement(thisBlockScope, child);
    }
  }
  
  void interpretStatement(Scope::Link currentScope, Node<ASTNode>::Link statement) {
    if (Node<DeclarationNode>::isSameType(statement)) {
      interpretDeclaration(currentScope, Node<DeclarationNode>::dynPtrCast(statement));
    } else if (Node<ExpressionNode>::isSameType(statement)) {
      auto obj = interpretExpression(currentScope, Node<ExpressionNode>::dynPtrCast(statement));
      println(obj->toString());
    } else if (Node<BranchNode>::isSameType(statement)) {
      interpretBranch(currentScope, Node<BranchNode>::dynPtrCast(statement));
    } else {
      throw InternalError("Unimplemented statement", {METADATA_PAIRS});
    }
  }
  
  void interpretBranch(Scope::Link currentScope, Node<BranchNode>::Link branch) {
    bool isConditionTrue = interpretExpression(currentScope, branch->getCondition())->isTruthy();
    if (isConditionTrue) {
      interpretBlock(currentScope, branch->getSuccessBlock());
      return;
    }
    auto failiureBlock = branch->getFailiureBlock();
    if (failiureBlock == nullptr) return;
    if (Node<BlockNode>::isSameType(failiureBlock)) {
      interpretBlock(currentScope, Node<BlockNode>::dynPtrCast(failiureBlock));
    } else if (Node<BranchNode>::isSameType(failiureBlock)) {
      interpretBranch(currentScope, Node<BranchNode>::dynPtrCast(failiureBlock));
    } else {
      throw InternalError("Wrong node type in branch block", {METADATA_PAIRS});
    }
  }
  
  void interpretDeclaration(Scope::Link currentScope, Node<DeclarationNode>::Link decl) {
    Object::Link init = decl->hasInit() ? interpretExpression(currentScope, decl->getInit()) : Object::Link();
    Reference::Link ref;
    if (decl->isDynamic()) ref = std::make_shared<Reference>(init);
    else ref = std::make_shared<Reference>(init, decl->getTypeInfo().getEvalTypeList());
    currentScope->insert(decl->getIdentifier(), ref);
  }
  
  Object::Link interpretExpression(Scope::Link currentScope, Node<ExpressionNode>::Link expr) {
    Token tok = expr->getToken();
    if (tok.isTerminal()) {
      switch (tok.type) {
        case L_INTEGER: return std::make_shared<Integer>(tok.data, 10);
        case L_FLOAT: return std::make_shared<Float>(tok.data);
        case L_STRING: return std::make_shared<String>(tok.data);
        case L_BOOLEAN: return std::make_shared<Boolean>(tok.data);
        case IDENTIFIER: return currentScope->get(tok.data).lock();
        default: throw InternalError("Unimplemented types in interpreter", {METADATA_PAIRS});
      };
    } else if (tok.isOperator()) {
      if (!tok.hasArity(Arity(expr->getChildren().size()))) {
        throw InternalError("Malformed expression node", {
          METADATA_PAIRS,
          {"expected children", std::to_string(tok.getOperator().getArity())},
          {"found children", std::to_string(expr->getChildren().size())}
        });
      }
      OperandList operands {};
      for (uint i = 0; i < expr->getChildren().size(); i++) {
        operands.push_back(interpretExpression(currentScope, expr->at(i)));
      }
      return executeOperator(operatorNameFrom(tok.idx), operands);
    }
    throw InternalError("Malformed expression node", {
      METADATA_PAIRS,
      {"token", tok.toString()}
    });
  }
public:
  void interpret(AST tree) {
    Scope::Link global = std::make_shared<Scope>();
    auto rootBlock = Node<BlockNode>::dynPtrCast(tree.getRoot());
    if (rootBlock == nullptr) throw InternalError("Root node is not a BlockNode", {METADATA_PAIRS});
    interpretBlock(global, rootBlock);
  }
};

#endif
