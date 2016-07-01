#ifndef INTERPRETER_HPP
#define INTERPRETER_HPP

#include <string>
#include <vector>

#include "ast.hpp"
#include "error.hpp"
#include "util.hpp"
#include "object.hpp"
#include "token.hpp"
#include "operatorMap.hpp"

class Interpreter {
private:
  AST input;
  
  void interpretBlock(Node<BlockNode>::Link block) {
    for (auto& child : block->getChildren()) {
      interpretStatement(child);
    }
  }
  
  void interpretStatement(Node<ASTNode>::Link statement) {
    if (Node<DeclarationNode>::isSameType(statement)) {
      interpretDeclaration(Node<DeclarationNode>::dynPtrCast(statement));
      return;
    } else if (Node<ExpressionNode>::isSameType(statement)) {
      auto obj = interpretExpression(Node<ExpressionNode>::dynPtrCast(statement));
      print(*PtrUtil<Integer>::dynPtrCast(obj));
      return;
    }
  }
  
  void interpretDeclaration(Node<DeclarationNode>::Link decl) {
    // TODO
  }
  
  Object::Link interpretExpression(Node<ExpressionNode>::Link expr) {
    Token tok = expr->getToken();
    if (tok.isTerminal()) {
      switch (tok.type) {
        case L_INTEGER: return PtrUtil<Integer>::make(tok.data, 10);
        case L_FLOAT: return PtrUtil<Float>::make(tok.data);
        case L_STRING: return PtrUtil<String>::make(tok.data);
        case L_BOOLEAN: return PtrUtil<Boolean>::make(tok.data);
        // TODO: IDENTIFIER case
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
        operands.push_back(interpretExpression(expr->at(i)));
      }
      return executeOperator(operatorNameFrom(tok.operatorIndex), operands);
    }
    throw InternalError("Malformed expression node", {
      METADATA_PAIRS,
      {"token", tok.toString()}
    });
  }
public:
  void interpret(AST tree) {
    this->input = tree;
    auto rootBlock = Node<BlockNode>::dynPtrCast(input.getRootAsLink());
    if (rootBlock == nullptr) throw InternalError("Root node is not a BlockNode", {METADATA_PAIRS});
    interpretBlock(rootBlock);
  }
};

#endif