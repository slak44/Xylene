#include "parser/tokenParser.hpp"

AST TokenParser::parse(std::vector<Token> list) {
  TokenParser tp = TokenParser();
  tp.input = list;
  return AST(tp.block(ROOT_BLOCK));
}

Node<ExpressionNode>::Link TokenParser::exprFromCurrent() {
  auto e = Node<ExpressionNode>::make(current());
  e->setTrace(current().trace);
  return e;
}

Node<ExpressionNode>::Link TokenParser::parseCircumfixGroup(Token begin) {
  const TokenType end =
    begin.type == TT::PAREN_LEFT ? TT::PAREN_RIGHT :
    begin.type == TT::CALL_BEGIN ? TT::CALL_END :
    begin.type == TT::SQPAREN_LEFT ? TT::SQPAREN_RIGHT :
      TT::UNPROCESSED;
  if (end == TT::UNPROCESSED)
    throw InternalError("Wrong token passed to parseCircumfixGroup", {
      METADATA_PAIRS,
      {"begin", begin.toString()}
    });
  // Allow empty expression only for calls
  bool throwIfEmpty = begin.type != TT::CALL_BEGIN;
  // Circumfix groups are guaranteed to be matched correctly by the lexer, so we only
  // need to keep track of how many times is our group opened. When we close
  // begin's group, beginOccurrences will be 0, because the begin token is skipped
  unsigned beginOccurrences = 1;
  const int beginPos = static_cast<int>(pos);
  std::size_t tokensInGroup = 0;
  while (beginOccurrences > 0) {
    if (accept(begin.type)) {
      beginOccurrences++;
    }
    if (accept(end)) {
      beginOccurrences--;
    }
    tokensInGroup++;
    skip();
  } // At this point the end token is already skipped
  // No tokens means empty expression
  if (tokensInGroup - 1 == 0) {
    if (throwIfEmpty) throw InternalError("Empty expression", {
      METADATA_PAIRS,
      {"circumfix group begin", begin.toString()}
    });
    return nullptr;
  }
  // This is like recursively calling expression(), but it has to be isolated from
  // the parent parser because it wouldn't know where to stop. Copying the correct
  // tokens ensures that only those are parsed, and doesn't mess with the parent's
  // internal state
  TokenParser recursive = TokenParser();
  // Copy everything between the beginning of the group and the end in the recursive
  // parser. beginPos is already after begin, but tokensInGroup counts the end, so -1
  recursive.input = std::vector<Token>(
    std::begin(input) + beginPos,
    std::begin(input) + beginPos + static_cast<int>(tokensInGroup) - 1
  );
  // Throw in a TT::FILE_END if there isn't any, so acceptEndOfExpression works
  if (recursive.input.back().type != TT::FILE_END)
    recursive.input.push_back(Token(TT::FILE_END, "", nullptr));
  return recursive.expression(throwIfEmpty);
}

Node<ExpressionNode>::Link TokenParser::parsePostfix(Node<ExpressionNode>::Link terminal) {
  Node<ExpressionNode>::Link base;
  // Check if there are any postfix operators around (function calls are postfix ops)
  while (accept(POSTFIX) || accept(TT::CALL_BEGIN)) {
    Node<ExpressionNode>::Link newOp;
    if (accept(POSTFIX)) {
      newOp = exprFromCurrent();
      skip();
    } else {
      newOp = Node<ExpressionNode>::make(
        Token(TT::OPERATOR, Operator::find("Call"), current().trace));
      Token begin = current();
      skip(); // Skip TT::CALL_BEGIN
      auto insideCall = parseCircumfixGroup(begin);
      auto args = Node<ExpressionNode>::make(
        Token(TT::OPERATOR, Operator::find("Call arguments"), current().trace));
      // Don't add any arguments to the call if the expression is empty
      if (insideCall == nullptr) {
        newOp->addChild(args);
        goto pseudoContinue;
      }
      // Add all arguments to function call
      auto lastNode = insideCall;
      // If it's not an comma, it means this func call only has one argument
      if (!(lastNode->getToken().isOp() && lastNode->getToken().op().hasSymbol(","))) {
        args->addChild(lastNode);
        newOp->addChild(args);
        goto pseudoContinue;
      }
      std::vector<Node<ExpressionNode>::Link> argList;
      // 2+ arguments
      while (lastNode->getToken().op().hasSymbol(",")) {
        argList.push_back(lastNode->at(1));
        auto tok = lastNode->at(0)->getToken();
        if (tok.type != TT::OPERATOR || !tok.op().hasSymbol(",")) break;
        lastNode = lastNode->at(0);
      }
      // The last comma's arg 2 is not processed in the loop
      argList.push_back(lastNode->at(0));
      // They come in reverse order due to depth-first above
      std::reverse(ALL(argList));
      for (auto arg : argList) {
        args->addChild(arg);
      }
      newOp->addChild(args);
    }
    // Using continue skips the code block below, which is not good, but we still
    // want continue-li
    pseudoContinue:
    if (base == nullptr) {
      base = newOp;
      base->addChild(terminal);
    } else {
      newOp->addChild(base);
      base = newOp;
    }
  }
  return base == nullptr ? terminal : base;
}

Node<ExpressionNode>::Link TokenParser::parseExpressionPrimary() {
  if (acceptEndOfExpression()) return nullptr; // Empty expression
  Node<ExpressionNode>::Link expr;
  if (accept(TT::CALL_BEGIN) || accept(TT::SQPAREN_LEFT))
    throw InternalError("The grammar does not allow this to be here", {
      METADATA_PAIRS,
      {"token", current().toString()}
    });
  if (accept(TT::PAREN_LEFT)) {
    Token begin = current();
    skip();
    expr = parseCircumfixGroup(begin);
    return parsePostfix(expr);
  } else if (acceptTerminal()) {
    expr = exprFromCurrent();
    skip();
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
    lastNode->addChild(terminalAndPostfix);
    return expr;
  }
  throw "Illegal token '{}' in expression"_syntax(current().data) + current().trace;
}

Node<ExpressionNode>::Link TokenParser::expressionImpl(Node<ExpressionNode>::Link lhs, int minPrecedence) {
  Node<ExpressionNode>::Link base = nullptr;
  
  Node<ExpressionNode>::Link lastExpr = nullptr;
  Token tok = current();
  if (acceptEndOfExpression()) return lhs;
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
    decl->init(expression());
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
  throw "Invalid declaration"_syntax + current().trace;
}

Node<BranchNode>::Link TokenParser::ifStatement() {
  auto branch = Node<BranchNode>::make();
  branch->setTrace(current().trace);
  branch->condition(expression());
  branch->success(block(IF_BLOCK));
  skip(-1); // Go back to the block termination token
  if (accept(TT::ELSE)) {
    skip();
    // Else-if structure
    if (accept(TT::IF)) {
      skip();
      branch->failiure<BranchNode>(ifStatement());
    // Simple else block
    } else if (accept(TT::DO)) {
      branch->failiure<BlockNode>(block(CODE_BLOCK));
    } else {
      throw "'else' must be followed by a 'do' or 'if'"_syntax + current().trace;
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
      throw "Expected comma after function argument '{}'"_syntax(current().data) + current().trace;
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
  if (isForeign && ident.empty()) {
    throw "Foreign functions can't be anonymous"_syntax + trace;
  }
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
  if (!isForeign) func->code(block(FUNCTION_BLOCK));
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
    constr->code(block(FUNCTION_BLOCK));
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
    methNode->code(parsedAsFunc->code());
  }
  return methNode;
}

Node<MemberNode>::Link TokenParser::member(Visibility vis, bool isStatic) {
  Trace mbTrace = current().trace;
  auto parsedAsDecl = declaration();
  auto mbNode = Node<MemberNode>::make(parsedAsDecl->getIdentifier(), parsedAsDecl->getTypeInfo().getEvalTypeList(), isStatic, vis == INVALID ? PRIVATE : vis);
  mbNode->setTrace(mbTrace);
  if (parsedAsDecl->getChildren().size() > 0)
    mbNode->init(parsedAsDecl->init());
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
      throw "Type '{}' body is not closed by 'end'"_syntax(identTok.data) + current().trace;
    }
    bool isStatic = false;
    bool isForeign = false;
    Visibility visibility = INVALID;
    // Expect to see a visibility_specifier or static or foreign
    while (accept(TT::PUBLIC) || accept(TT::PRIVATE) || accept(TT::PROTECT) || accept(TT::STATIC) || accept(TT::FOREIGN)) {
      if (accept(TT::STATIC)) {
        if (isStatic == true) {
          throw "Cannot specify 'static' more than once"_syntax + current().trace;
        }
        isStatic = true;
      } else if (accept(TT::FOREIGN)) {
        if (isForeign == true) {
          throw "Cannot specify 'foreign' more than once"_syntax + current().trace;
        }
        isForeign = true;
      } else {
        if (visibility != INVALID) {
          throw "Cannot have more than one visibility specifier"_syntax + current().trace;
        }
        visibility = fromToken(current());
      }
      skip();
    }
    // Handle things that go in the body
    if (accept(TT::CONSTR)) {
      if (isStatic)
        throw "Constructors can't be static"_syntax + current().trace;
      tn->addChild(constructor(visibility, isForeign));
    } else if (accept(TT::METHOD)) {
      if (visibility == INVALID)
        throw "Methods require a visibility specifier"_syntax + current().trace;
      tn->addChild(method(visibility, isStatic, isForeign));
    } else {
      if (isForeign)
        throw "Member fields can't be foreign"_syntax + current().trace;
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
    // TODO: multiple decls
    loop->addInit(declaration(false));
    expectSemi();
    loop->condition(expression(false));
    expectSemi();
    // Tree with commas and updates
    auto updates = expression(false);
    loop->code(block(CODE_BLOCK));
    // No updates to parse
    if (updates == nullptr) {
      return loop;
    }
    // No comma tree means we have only one update
    if (!updates->getToken().op().hasSymbol(",")) {
      loop->addUpdate(updates);
      return loop;
    }
    // Parse comma tree
    std::vector<Node<ExpressionNode>::Link> splitUpdates;
    while (updates->getToken().op().hasSymbol(",")) {
      splitUpdates.push_back(updates->at(1));
      if (!updates->at(0)->getToken().op().hasSymbol(",")) break;
      updates = updates->at(0);
    }
    // Bottom of the tree has another update instead of a comma
    splitUpdates.push_back(updates->at(0));
    // The above process adds the upds in reverse, so flip the vector
    std::reverse(ALL(splitUpdates));
    for (auto upd : splitUpdates) {
      loop->addUpdate(upd);
    }
    return loop;
  } if (accept(TT::WHILE)) {
    skip(); // Skip "while"
    auto loop = Node<LoopNode>::make();
    loop->setTrace(current().trace);
    loop->condition(expression());
    loop->code(block(CODE_BLOCK));
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
    if (retValue != nullptr) retNode->value(retValue);
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
