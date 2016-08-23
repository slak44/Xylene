#include "parser/tokenParser.hpp"

bool TokenBaseParser::expect(TokenType tok, std::string errorMessage) {
  if (accept(tok)) {
    return true;
  } else {
    auto currentData = current().isOperator() ? current().getOperator().getName() : current().data;
    throw Error("SyntaxError", errorMessage + " (found: " + currentData + ")", current().trace);
  }
}

void TokenBaseParser::expectSemi() {
  expect(C_SEMI, "Expected semicolon");
  skip();
}

TokenParser& TokenParser::parse(std::vector<Token> input) {
  this->input = input;
  this->pos = 0;
  tree = std::make_unique<AST>(AST(block(ROOT_BLOCK)));
  return *this;
}

BlockParser::BlockParser(StatementParser* stp): stp(stp) {}
IfStatementParser::IfStatementParser(StatementParser* stp): BlockParser(stp) {}
FunctionParser::FunctionParser(StatementParser* stp): BlockParser(stp) {}
StatementParser::StatementParser(): BlockParser(this), IfStatementParser(this), FunctionParser(this) {}
TokenParser::TokenParser(): BlockParser(this), IfStatementParser(this), FunctionParser(this) {}

Node<ExpressionNode>::Link ExpressionParser::exprFromCurrent() {
  auto e = Node<ExpressionNode>::make(current());
  e->setTrace(current().trace);
  return e;
}

Node<ExpressionNode>::Link ExpressionParser::parseExpressionPrimary() {
  if (accept(C_SEMI) || accept(K_DO)) return nullptr; // Empty expression
  Node<ExpressionNode>::Link expr;
  if (accept(C_PAREN_LEFT)) {
    skip();
    expr = expression();
    expect(C_PAREN_RIGHT, "Mismatched parenthesis");
    skip();
    return expr;
  } else if (acceptTerminal()) {
    auto terminal = Node<ExpressionNode>::make(current());
    skip();
    // Check if there are any postfix operators around
    if (accept(POSTFIX)) {
      auto lastNode = expr = exprFromCurrent();
      skip();
      while (accept(POSTFIX)) {
        auto newPostfix = exprFromCurrent();
        lastNode->addChild(newPostfix);
        lastNode = newPostfix;
        skip();
      }
      lastNode->addChild(terminal);
    } else {
      expr = terminal;
    }
    return expr;
  // Prefix operators
  } else if (accept(PREFIX)) {
    auto lastNode = expr = exprFromCurrent();
    skip();
    while (accept(PREFIX)) {
      lastNode->addChild(exprFromCurrent());
      lastNode = lastNode->at(-1);
      skip();
    }
    lastNode->addChild(parseExpressionPrimary());
    return expr;
  } else {
    throw InternalError("Unimplemented primary expression", {
      METADATA_PAIRS,
      {"token", current().toString()},
      {"token pos", std::to_string(pos)}
    });
  }
}

Node<ExpressionNode>::Link ExpressionParser::expressionImpl(Node<ExpressionNode>::Link lhs, int minPrecedence) {
  Node<ExpressionNode>::Link base = nullptr;
  
  Node<ExpressionNode>::Link lastExpr = nullptr;
  Token tok = current();
  if (acceptEndOfExpression()) return lhs;
  while (tok.hasArity(BINARY) && tok.getPrecedence() >= minPrecedence) {
    auto tokExpr = Node<ExpressionNode>::make(tok);
    tokExpr->setTrace(tok.trace);
    tokExpr->addChild(lhs);
    skip();
    auto rhs = parseExpressionPrimary();
    tok = current();
    while (tok.isOperator() && tok.hasArity(BINARY) &&
      (
        tok.getPrecedence() <= tokExpr->getToken().getPrecedence() ||
        (tok.getPrecedence() == tokExpr->getToken().getPrecedence() && tok.hasAssociativity(ASSOCIATE_FROM_RIGHT))
      )
    ) {
      tokExpr->addChild(rhs);
      tokExpr = expressionImpl(tokExpr, tok.getPrecedence());
      rhs = nullptr;
      tok = current();
    }
    lhs = rhs;
    if (base == nullptr) {
      base = lastExpr = tokExpr;
    } else {
      lastExpr->addChild(tokExpr);
      lastExpr = tokExpr;
    }
    if (acceptEndOfExpression()) break;
  }
  if (base == nullptr) {
    base = lhs;
  } else if (lhs != nullptr) {
    lastExpr->addChild(lhs);
  }
  return base;
}

Node<ExpressionNode>::Link ExpressionParser::expression(bool throwIfEmpty) {
  auto primary = parseExpressionPrimary();
  if (primary == nullptr) {
    if (throwIfEmpty) throw InternalError("Empty expression", {METADATA_PAIRS});
    else return nullptr;
  }
  return expressionImpl(primary, 0);
}

Node<DeclarationNode>::Link DeclarationParser::declarationFromTypes(TypeList typeList) {
  Token identToken = current();
  skip();
  auto decl = Node<DeclarationNode>::make(identToken.data, typeList);
  decl->setTrace(identToken.trace);
  // Do initialization only if it exists
  if (accept("=")) {
    skip();
    decl->setInit(expression());
  }
  return decl;
}

Node<DeclarationNode>::Link DeclarationParser::declaration(bool throwIfEmpty) {
  if (accept(C_SEMI)) {
    if (throwIfEmpty) throw InternalError("Empty declaration", {METADATA_PAIRS});
    else return nullptr;
  }
  if (accept(K_DEFINE)) {
    skip();
    // Dynamic variable declaration
    if (accept(IDENTIFIER)) {
      return declarationFromTypes({});
    // Function declaration
    } else if (accept(K_FUNCTION)) {
      skip();
      // TODO
      throw InternalError("Unimplemented", {METADATA_PAIRS, {"token", "function def"}});
    } else {
      throw Error("SyntaxError", "Unexpected token after define keyword", current().trace);
    }
  } else if (accept(IDENTIFIER)) {
    auto ident = current().data;
    skip();
    // Single-type declaration
    if (accept(IDENTIFIER)) {
      return declarationFromTypes({ident});
    }
    // Multi-type declaration
    if (accept(",")) {
      TypeList types = {ident};
      do {
        skip();
        expect(IDENTIFIER, "Expected identifier in type list");
        types.insert(current().data);
        skip();
      } while (accept(","));
      expect(IDENTIFIER);
      return declarationFromTypes(types);
    }
  }
  throw InternalError("Invalid declaration", {METADATA_PAIRS});
}

Node<BranchNode>::Link IfStatementParser::ifStatement() {
  auto branch = Node<BranchNode>::make();
  branch->setTrace(current().trace);
  branch->setCondition(expression());
  branch->setSuccessBlock(block(IF_BLOCK));
  skip(-1); // Go back to the block termination token
  if (accept(K_ELSE)) {
    skip();
    // Else-if structure
    if (accept(K_IF)) {
      skip();
      branch->setFailiureBlock(ifStatement());
    // Simple else block
    } else if (accept(K_DO)) {
      branch->setFailiureBlock(block(CODE_BLOCK));
    } else {
      throw Error("SyntaxError", "Else must be followed by a block or an if statement", current().trace);
    }
  }
  return branch;
}

TypeList FunctionParser::getTypeList() {
  TypeList types = {};
  skip(-1); // Counter the comma skip below for the first iteration
  do {
    skip(1); // Skips the comma
    expect(IDENTIFIER, "Expected identifier in type list");
    types.insert(current().data);
    skip();
  } while (accept(","));
  return types;
}

Node<FunctionNode>::Link FunctionParser::function() {
  Trace trace = current().trace;
  skip(); // Skip "function"
  std::string ident = "";
  FunctionSignature::Arguments args {};
  std::unique_ptr<TypeInfo> returnType;
  // Is not anon func
  if (accept(IDENTIFIER)) {
    ident = current().data;
    skip();
  }
  // Has arguments
  if (accept(C_SQPAREN_LEFT)) {
    skip();
    while (true) {
      expect(IDENTIFIER, "Expected identifier in function arguments");
      Token ident = current();
      skip();
      // argName: typeList
      if (accept(C_2POINT)) {
        skip();
        args.insert(std::make_pair(ident.data, getTypeList()));
      // typeList argName
      } else {
        skip(-1); // Don't eat the first identifier
        TypeList tl = getTypeList();
        args.insert(std::make_pair(current().data, tl));
        skip(); // Skip the argument name
      }
      if (accept(C_SQPAREN_RIGHT)) {
        skip();
        break;
      }
      if (!accept(",")) {
        throw Error("SyntaxError", "Expected comma after function argument", current().trace);
      }
      skip(); // The comma
    }
  }
  // Has return type
  if (accept(K_FAT_ARROW)) {
    skip();
    returnType = std::make_unique<TypeInfo>(TypeInfo(getTypeList()));
  }
  auto func = Node<FunctionNode>::make(ident, FunctionSignature(returnType == nullptr ? nullptr : *returnType, args));
  func->setTrace(trace);
  func->setCode(block(FUNCTION_BLOCK));
  return func;
}

ASTNode::Link StatementParser::statement() {
  if (accept(K_IF)) {
    skip();
    return ifStatement();
  } else if (accept(K_FOR)) {
    auto loop = Node<LoopNode>::make();
    loop->setTrace(current().trace);
    skip(); // Skip "for"
    loop->setInit(declaration(false));
    expectSemi();
    loop->setCondition(expression(false));
    expectSemi();
    loop->setUpdate(expression(false));
    loop->setCode(block(CODE_BLOCK));
    return loop;
  } if (accept(K_WHILE)) {
    skip(); // Skip "while"
    auto loop = Node<LoopNode>::make();
    loop->setTrace(current().trace);
    loop->setCondition(expression());
    loop->setCode(block(CODE_BLOCK));
    return loop;
  } else if (accept(K_DO)) {
    return block(CODE_BLOCK);
  } else if (accept(K_DEFINE)) {
    auto decl = declaration();
    expectSemi();
    return decl;
  } else if (accept(IDENTIFIER)) {
    skip();
    if (accept(IDENTIFIER) || accept(",")) {
      skip(-1); // Go back to the prev identifier
      auto decl = declaration();
      expectSemi();
      return decl;
    } else {
      skip(-1); // Get the entire expression
      auto e = expression();
      expectSemi();
      return e;
    }
  } else if (accept(K_BREAK)) {
    skip();
    return Node<BreakLoopNode>::make();
  } else if (accept(K_CONTINUE)) {
    throw InternalError("Unimplemented", {METADATA_PAIRS, {"token", "loop continue"}});
  } else if (accept(K_RETURN)) {
    skip(); // Skip "return"
    auto retValue = expression();
    expectSemi();
    auto retNode = Node<ReturnNode>::make();
    retNode->setValue(retValue);
    return retNode;
  } else if (accept(K_FUNCTION)) {
    return function();
  } else {
    auto e = expression();
    expectSemi();
    return e;
  }
}

Node<BlockNode>::Link BlockParser::block(BlockType type) {
  if (type != ROOT_BLOCK) {
    expect(K_DO, "Expected code block");
    skip();
  }
  Node<BlockNode>::Link block = Node<BlockNode>::make(type);
  block->setTrace(current().trace);
  while (!accept(K_END)) {
    if (type == IF_BLOCK && accept(K_ELSE)) break;
    if (type == ROOT_BLOCK && accept(FILE_END)) break;
    block->addChild(stp->statement());
  }
  skip(); // Skip block end
  return block;
}
