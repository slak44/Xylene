#include "parser/tokenParser.hpp"

TokenParser& TokenParser::parse(std::vector<Token> input) {
  this->input = input;
  this->pos = 0;
  tree = std::make_unique<AST>(AST(block(ROOT_BLOCK)));
  return *this;
}

Node<ExpressionNode>::Link TokenParser::exprFromCurrent() {
  auto e = Node<ExpressionNode>::make(current());
  e->setTrace(current().trace);
  return e;
}

Node<ExpressionNode>::Link TokenParser::parsePostfix(Node<ExpressionNode>::Link terminal) {
  Node<ExpressionNode>::Link expr;
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
}

Node<ExpressionNode>::Link TokenParser::parseExpressionPrimary(bool parenAsFuncCall = false) {
  if (acceptEndOfExpression()) return nullptr; // Empty expression
  Node<ExpressionNode>::Link expr;
  if (accept(TT::PAREN_LEFT)) {
    skip();
    expr = expression(false);
    expect(TT::PAREN_RIGHT, "Mismatched parenthesis");
    if (
      // Something higher up the chain has higher level info forcing this parse path
      parenAsFuncCall ||
      // Empty expression between parens means it's actually a function call without arguments
      expr == nullptr ||
      // If it's a terminal, it's a function call with 1 arg
      expr->getToken().isTerminal() ||
      // If the root of the parenthesised expression is a comma, then this is a tree of arguments, so func call
      (expr->getToken().isOp() && expr->getToken().op().hasSymbol(","))
    ) {
      auto callOpExpr = Node<ExpressionNode>::make(
        Token(TT::OPERATOR, Operator::find("Call"), current().trace)
      );
      // If expr is nullptr, use a no-op
      callOpExpr->addChild(expr != nullptr ? expr :
        Node<ExpressionNode>::make(Token(TT::OPERATOR, Operator::find("No-op"), current().trace)));
      expr = callOpExpr;
    }
    skip(); // Skip ")"
    expr = parsePostfix(expr);
    return expr;
  } else if (acceptTerminal()) {
    expr = exprFromCurrent();
    skip();
    // Function call
    if (expr->getToken().type == TT::IDENTIFIER && accept(TT::PAREN_LEFT)) {
      auto call = parseExpressionPrimary(true);
      // The expr being called must be the first operand of the call operator
      // So remove the arguments, add expr, then put the arguments back in
      auto argTree = call->removeChild(0);
      call->addChild(expr);
      call->addChild(argTree);
      return call;
    }
    return parsePostfix(expr);
  // Prefix operators
  } else if (accept(PREFIX)) {
    auto lastNode = expr = exprFromCurrent();
    skip();
    while (accept(PREFIX)) {
      lastNode->addChild(exprFromCurrent());
      lastNode = lastNode->at(-1);
      skip();
    }
    auto terminalAndPostfix = parseExpressionPrimary();
    // Postfix ops must get in front if at least one exists
    if (
      terminalAndPostfix->getToken().isOp() && // Check that there is a postfix op, not a terminal
      lastNode->getToken().op().getPrec() < terminalAndPostfix->getToken().op().getPrec()
    ) {
      auto lastPostfix = terminalAndPostfix;
      // Go down the tree and find the last postfix
      while (lastPostfix->getToken().isOp() && lastPostfix->getToken().op().hasFixity(POSTFIX)) {
        lastPostfix = lastPostfix->at(0);
      }
      // This loop iterates one too many times, and finds the terminal
      // The last postfix is the terminal's parent
      lastPostfix = Node<ExpressionNode>::staticPtrCast(lastPostfix->getParent().lock());
      // Add the terminal to the prefixes
      lastNode->addChild(lastPostfix->removeChild(0));
      // Add the prefixes as a child of the last postfix
      lastPostfix->addChild(expr);
      // The postfixes are the root of the returned expr node
      expr = terminalAndPostfix;
    } else {
      lastNode->addChild(terminalAndPostfix);
    }
    return expr;
  } else {
    throw InternalError("Unimplemented primary expression", {
      METADATA_PAIRS,
      {"token", current().toString()},
      {"token pos", std::to_string(pos)}
    });
  }
}

Node<ExpressionNode>::Link TokenParser::expressionImpl(Node<ExpressionNode>::Link lhs, int minPrecedence) {
  Node<ExpressionNode>::Link base = nullptr;
  
  Node<ExpressionNode>::Link lastExpr = nullptr;
  Token tok = current();
  if (acceptEndOfExpression()) return lhs;
  if (lhs != nullptr) {
    skip(-1);
    auto tt = current().type;
    skip(1);
    // If we have any of:
    // TT::IDENTIFIER, TT::PAREN_RIGHT, postfix ops
    // before a paren, it's a function call
    auto isCallable =
      tt == TT::IDENTIFIER ||
      tt == TT::PAREN_RIGHT ||
      (tt == TT::OPERATOR && lhs->getToken().op().hasFixity(POSTFIX));
    if (accept(TT::PAREN_LEFT) && isCallable) {
      auto fCall = parseExpressionPrimary(true);
      auto args = fCall->removeChild(0);
      fCall->addChild(lhs);
      fCall->addChild(args);
      lhs = fCall;
      // If the expression finishes here, return
      if (acceptEndOfExpression()) return lhs;
      // Otherwise, update tok
      tok = current();
    }
  }
  while (tok.op().hasArity(BINARY) && tok.op().getPrec() >= minPrecedence) {
    auto tokExpr = Node<ExpressionNode>::make(tok);
    tokExpr->setTrace(tok.trace);
    tokExpr->addChild(lhs);
    skip();
    auto rhs = parseExpressionPrimary();
    tok = current();
    while (tok.isOp() && tok.op().hasArity(BINARY) &&
      (
        tok.op().getPrec() <= tokExpr->getToken().op().getPrec() ||
        (tok.op().getPrec() == tokExpr->getToken().op().getPrec() && tok.op().hasAsoc(ASSOCIATE_FROM_RIGHT))
      )
    ) {
      tokExpr->addChild(rhs);
      tokExpr = expressionImpl(tokExpr, tok.op().getPrec());
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

Node<ExpressionNode>::Link TokenParser::expression(bool throwIfEmpty) {
  auto primary = parseExpressionPrimary();
  if (primary == nullptr) {
    if (throwIfEmpty) throw InternalError("Empty expression", {
      METADATA_PAIRS,
      {"token", current().toString()}
    });
    else return nullptr;
  }
  return expressionImpl(primary, 0);
}

Node<DeclarationNode>::Link TokenParser::declarationFromTypes(TypeList typeList) {
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

Node<DeclarationNode>::Link TokenParser::declaration(bool throwIfEmpty) {
  if (accept(TT::SEMI)) {
    if (throwIfEmpty) throw InternalError("Empty declaration", {METADATA_PAIRS});
    else return nullptr;
  }
  if (accept(TT::DEFINE)) {
    skip();
    // Dynamic variable declaration
    expect(TT::IDENTIFIER, "Unexpected token after define keyword");
    return declarationFromTypes({});
  } else if (accept(TT::IDENTIFIER)) {
    auto ident = current().data;
    skip();
    // Single-type declaration
    if (accept(TT::IDENTIFIER)) {
      return declarationFromTypes({ident});
    }
    // Multi-type declaration
    if (accept(",")) {
      TypeList types = {ident};
      do {
        skip();
        expect(TT::IDENTIFIER, "Expected identifier in type list");
        types.insert(current().data);
        skip();
      } while (accept(","));
      expect(TT::IDENTIFIER);
      return declarationFromTypes(types);
    }
  }
  throw Error("SyntaxError", "Invalid declaration", current().trace);
}

Node<BranchNode>::Link TokenParser::ifStatement() {
  auto branch = Node<BranchNode>::make();
  branch->setTrace(current().trace);
  branch->setCondition(expression());
  branch->setSuccessBlock(block(IF_BLOCK));
  skip(-1); // Go back to the block termination token
  if (accept(TT::ELSE)) {
    skip();
    // Else-if structure
    if (accept(TT::IF)) {
      skip();
      branch->setFailiureBlock(ifStatement());
    // Simple else block
    } else if (accept(TT::DO)) {
      branch->setFailiureBlock(block(CODE_BLOCK));
    } else {
      throw Error("SyntaxError", "Else must be followed by a block or an if statement", current().trace);
    }
  }
  return branch;
}

TypeList TokenParser::getTypeList() {
  TypeList types = {};
  skip(-1); // Counter the comma skip below for the first iteration
  do {
    skip(1); // Skips the comma
    expect(TT::IDENTIFIER, "Expected identifier in type list");
    types.insert(current().data);
    skip();
  } while (accept(","));
  return types;
}

FunctionSignature::Arguments TokenParser::getSigArgs() {
  FunctionSignature::Arguments args;
  while (true) {
    expect(TT::IDENTIFIER, "Expected identifier in function arguments");
    TypeList tl = getTypeList();
    args.push_back(std::make_pair(current().data, tl));
    skip(); // Skip the argument name
    if (accept(TT::SQPAREN_RIGHT)) {
      skip();
      break;
    }
    if (!accept(",")) {
      throw Error("SyntaxError", "Expected comma after function argument", current().trace);
    }
    skip(); // The comma
  }
  return args;
}

Node<FunctionNode>::Link TokenParser::function(bool isForeign) {
  if (isForeign) skip(); // Skip "foreign"
  Trace trace = current().trace;
  skip(); // Skip "function" or "method"
  std::string ident = "";
  FunctionSignature::Arguments args {};
  std::unique_ptr<TypeInfo> returnType;
  // Is not anon func
  if (accept(TT::IDENTIFIER)) {
    ident = current().data;
    skip();
  }
  if (isForeign && ident.empty()) throw Error("SyntaxError", "Foreign functions can't be anonymous", trace);
  // Has arguments
  if (accept(TT::SQPAREN_LEFT)) {
    skip();
    args = getSigArgs();
  }
  // Has return type
  if (accept(TT::FAT_ARROW)) {
    skip();
    returnType = std::make_unique<TypeInfo>(getTypeList());
  }
  auto func = Node<FunctionNode>::make(ident, FunctionSignature(returnType == nullptr ? nullptr : *returnType, args), isForeign);
  func->setTrace(trace);
  // Only non-foreign functions have code bodies
  if (!isForeign) func->setCode(block(FUNCTION_BLOCK));
  // Foreign declarations end in semicolon
  if (isForeign) expectSemi();
  return func;
}

Node<ConstructorNode>::Link TokenParser::constructor(Visibility vis, bool isForeign) {
  Trace constrTrace = current().trace;
  skip(); // Skip "constructor"
  FunctionSignature::Arguments args {};
  // Has arguments
  if (accept(TT::SQPAREN_LEFT)) {
    skip();
    args = getSigArgs();
  }
  auto constr = Node<ConstructorNode>::make(args, vis == INVALID ? PUBLIC : vis, isForeign);
  constr->setTrace(constrTrace);
  if (isForeign) {
    expectSemi();
  } else {
    constr->setCode(block(FUNCTION_BLOCK));    
  }
  return constr;
}

Node<MethodNode>::Link TokenParser::method(Visibility vis, bool isStatic, bool isForeign) {
  Trace methTrace = current().trace;
  auto parsedAsFunc = function(isForeign);
  auto methNode = Node<MethodNode>::make(parsedAsFunc->getIdentifier(), parsedAsFunc->getSignature(), vis, isStatic);
  methNode->setTrace(methTrace);
  if (isForeign) {
    expectSemi();
  } else {
    methNode->setCode(Node<BlockNode>::staticPtrCast(parsedAsFunc->removeChild(0)));
  }
  return methNode;
}

Node<MemberNode>::Link TokenParser::member(Visibility vis, bool isStatic) {
  Trace mbTrace = current().trace;
  auto parsedAsDecl = declaration();
  auto mbNode = Node<MemberNode>::make(parsedAsDecl->getIdentifier(), parsedAsDecl->getTypeInfo().getEvalTypeList(), isStatic, vis == INVALID ? PRIVATE : vis);
  mbNode->setTrace(mbTrace);
  if (parsedAsDecl->getChildren().size() > 0) mbNode->setInit(Node<ExpressionNode>::staticPtrCast(parsedAsDecl->removeChild(0)));
  return mbNode;
}

Node<TypeNode>::Link TokenParser::type() {
  skip(); // Skip "type"
  expect(TT::IDENTIFIER, "Expected identifier after 'type' token");
  Node<TypeNode>::Link tn;
  Token identTok = current();
  skip();
  if (accept(TT::INHERITS)) {
    skip();
    if (accept(TT::FROM)) skip(); // Having "from" after "inherits" is optional
    tn = Node<TypeNode>::make(identTok.data, getTypeList());
  } else {
    tn = Node<TypeNode>::make(identTok.data);
  }
  tn->setTrace(identTok.trace);
  expect(TT::DO, "Expected type body");
  skip();
  while (!accept(TT::END)) {
    if (accept(TT::FILE_END)) {
      skip(-1); // Go back to get a prettier trace
      throw Error("SyntaxError", "Type body is not closed by 'end'", current().trace);
    }
    bool isStatic = false;
    bool isForeign = false;
    Visibility visibility = INVALID;
    // Expect to see a visibility_specifier or static or foreign
    while (accept(TT::PUBLIC) || accept(TT::PRIVATE) || accept(TT::PROTECT) || accept(TT::STATIC) || accept(TT::FOREIGN)) {
      if (accept(TT::STATIC)) {
        if (isStatic == true) throw Error("SyntaxError", "Cannot specify 'static' more than once", current().trace);
        isStatic = true;
      } else if (accept(TT::FOREIGN)) {
        if (isForeign == true) throw Error("SyntaxError", "Cannot specify 'foreign' more than once", current().trace);
        isForeign = true;
      } else {
        if (visibility != INVALID) throw Error("SyntaxError", "Cannot have more than one visibility specifier", current().trace);
        visibility = fromToken(current());
      }
      skip();
    }
    // Handle things that go in the body
    if (accept(TT::CONSTR)) {
      if (isStatic) throw Error("SyntaxError", "Constructors can't be static", current().trace);
      tn->addChild(constructor(visibility, isForeign));
    } else if (accept(TT::METHOD)) {
      if (visibility == INVALID) throw Error("SyntaxError", "Methods require a visibility specifier", current().trace);
      tn->addChild(method(visibility, isStatic, isForeign));
    } else {
      if (isForeign) throw Error("SyntaxError", "Member fields can't be foreign", current().trace);
      tn->addChild(member(visibility, isStatic));
      expectSemi();
    }
  }
  return tn;
}

ASTNode::Link TokenParser::statement() {
  if (accept(TT::TYPE)) {
    return type();
  } else if (accept(TT::IF)) {
    skip();
    return ifStatement();
  } else if (accept(TT::FOR)) {
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
  } if (accept(TT::WHILE)) {
    skip(); // Skip "while"
    auto loop = Node<LoopNode>::make();
    loop->setTrace(current().trace);
    loop->setCondition(expression());
    loop->setCode(block(CODE_BLOCK));
    return loop;
  } else if (accept(TT::DO)) {
    return block(CODE_BLOCK);
  } else if (accept(TT::DEFINE)) {
    auto decl = declaration();
    expectSemi();
    return decl;
  } else if (accept(TT::IDENTIFIER)) {
    skip();
    if (accept(TT::IDENTIFIER) || accept(",")) {
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
  } else if (accept(TT::BREAK)) {
    skip();
    return Node<BreakLoopNode>::make();
  } else if (accept(TT::CONTINUE)) {
    throw InternalError("Unimplemented", {METADATA_PAIRS, {"token", "loop continue"}});
  } else if (accept(TT::RETURN)) {
    auto trace = current().trace;
    skip(); // Skip "return"
    auto retValue = expression(false);
    expectSemi();
    auto retNode = Node<ReturnNode>::make();
    retNode->setTrace(trace);
    if (retValue != nullptr) retNode->setValue(retValue);
    return retNode;
  } else if (accept(TT::FUNCTION)) {
    return function();
  } else if (accept(TT::FOREIGN)) {
    return function(true);
  } else {
    auto e = expression();
    expectSemi();
    return e;
  }
}

Node<BlockNode>::Link TokenParser::block(BlockType type) {
  if (type != ROOT_BLOCK) {
    expect(TT::DO, "Expected code block");
    skip();
  }
  Node<BlockNode>::Link block = Node<BlockNode>::make(type);
  block->setTrace(current().trace);
  while (!accept(TT::END)) {
    if (type == IF_BLOCK && accept(TT::ELSE)) break;
    if (type == ROOT_BLOCK && accept(TT::FILE_END)) break;
    block->addChild(statement());
  }
  skip(); // Skip block end
  return block;
}
