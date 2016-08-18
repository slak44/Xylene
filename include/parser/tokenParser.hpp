#ifndef TOKEN_PARSER_HPP
#define TOKEN_PARSER_HPP

#include <string>
#include <vector>
#include <algorithm>

#include "utils/util.hpp"
#include "utils/error.hpp"
#include "parser/baseParser.hpp"
#include "ast.hpp"
#include "token.hpp"
#include "operator.hpp"

// The compiler was whining about some move assignments for virtual bases
// The classes it mentioned don't actually have anything to assign, so I don't care
#pragma GCC diagnostic ignored "-Wvirtual-move-assign"

/*
  EBNF-ish format of a program:
  
  program = block ;
  block = [ statement, ";", { statement, ";" } ] ;
  statement = declaration | ( "define", function ) | for_loop | while_loop | block | if_statement | try_catch | throw_statement | expression ;
  declaration = "define" | type_list, ident, [ "=", expression ] ;
  for_loop = "for", expression, ";", expression, ";", expression, "do", block, "end" ;
  while_loop = "while", expression, "do", block, "end" ;
  if_statement = "if", expression, "do", block, [ "else", block | if_statement ] | "end" ;
  type_definition = "define", "type", ident, [ "inherits", type_list ], "do", [ contructor_definition ], [ { method_definition | member_definition } ], "end" ;
  constructor_definition = "define", "constructor", [ argument, {",", argument } ], "do", block, "end" ;
  method_definition = "define", [ visibility_specifier ], [ "static" ], function ;
  member_definition = "define", [ visibility_specifier ], [ "static" ], ident, [ "=", expression ] ;
  try_catch = "try", block, "catch", type_list, ident, "do", block, "end" ;
  throw_statement = "throw", expression ;
  
  function = "function", ident, [ argument, {",", argument } ], [ "=>", type_list ], "do", block, "end" ;
  visibility_specifier = "public" | "private" | "protected" ;
  type_list = ident, {",", ident} ;
  argument = ( "[", type_list, ident, "]" ) | ( "[", ident, ":", type_list, "]" ) ;
  expression = primary, { binary_op, primary } ;
  primary = { prefix_op }, terminal, { postfix_op } ;
  binary_op = ? binary operator ? ;
  prefix_op = ? prefix operator ? ;
  postfix_op = ? postfix operator ? ;
  terminal = ? see Token's isTerminal method ? ;
  function_call = ident, "(", expression, { ",", expression }, ")" ;
  ident = ? any valid identifier ? ;
*/

class TokenBaseParser: public BaseParser {
protected:
  std::vector<Token> input;
  uint64 pos = 0;
  
  TokenBaseParser() {}
  
  // Current token
  inline Token current() {
    return input[pos];
  }
  // Skip a number of tokens, usually just advances to the next one
  inline void skip(int by = 1) {
    pos += by;
  }
  // Accept Tokens with certain properties
  inline bool accept(TokenType tok) {
    return tok == current().type;
  }
  inline bool accept(OperatorSymbol operatorSymbol) {
    if (!current().isOperator()) return false;
    if (current().hasOperatorName(operatorSymbol)) return true;
    return false;
  }
  inline bool accept(Fixity fixity) {
    return current().isOperator() && current().hasFixity(fixity);
  }
  inline bool acceptTerminal() {
    return current().isTerminal();
  }
  inline bool acceptEndOfExpression() {
    return accept(C_SEMI) || accept(C_PAREN_RIGHT) || accept(FILE_END) || accept(K_DO);
  }
  // Expect certain tokens to appear (throw otherwise)
  bool expect(TokenType tok, std::string errorMessage = "Unexpected symbol");
  void expectSemi();
};

// Contains methods for parsing expressions in a parser
class ExpressionParser: virtual public TokenBaseParser {
protected:
  ExpressionParser() {}
private:
  // Creates an ExpressionNode from the current Token
  Node<ExpressionNode>::Link exprFromCurrent();
  // Parse a primary in the expression
  // Returns an ExpressionNode, or nullptr if the expression is empty (terminates immediately)
  // For example, ';' is an empty expression
  Node<ExpressionNode>::Link parseExpressionPrimary();
  // Implementation detail of expression
  // This method exists to allow recursion
  Node<ExpressionNode>::Link expressionImpl(Node<ExpressionNode>::Link lhs, int minPrecedence);
public:
  // Parse an expression starting at the current token
  // throwIfEmpty: throws an error on empty expressions; if set to false, empty expressions return nullptr
  Node<ExpressionNode>::Link expression(bool throwIfEmpty = true);
};

// Contains methods for parsing declarations in a parser
class DeclarationParser: virtual public ExpressionParser {
protected:
  DeclarationParser() {}
private:
  // Creates a DeclarationNode from a list of types, handles initialization if available
  Node<DeclarationNode>::Link declarationFromTypes(TypeList typeList);
public:
  // Parse a declaration starting at the current token
  // throwIfEmpty: throws an error on empty declarations; if set to false, empty declarations return nullptr
  Node<DeclarationNode>::Link declaration(bool throwIfEmpty = true);
};

class StatementParser;

class BlockParser: virtual public TokenBaseParser {
protected:
  // This avoids circular inheritance between BlockParser and StatementParser
  StatementParser* stp;
  BlockParser(StatementParser* stp);
public:
  // Parse a block, depending on its type
  // Normal code blocks are enclosed in K_DO/K_END
  // Root blocks aren't expected to be enclosed, and instead they end at the file end
  // If blocks can also be ended with K_ELSE besides K_END
  Node<BlockNode>::Link block(BlockType type);
};

class IfStatementParser: virtual public ExpressionParser, virtual public BlockParser {
protected:
  // See BlockParser's ctor
  IfStatementParser(StatementParser* stp);
public:
  // Parse an if statement starting at the current token
  // This function assumes K_IF has been skipped, however
  Node<BranchNode>::Link ifStatement();
};

class StatementParser: virtual public IfStatementParser, virtual public DeclarationParser {
protected:
  // See BlockParser's ctor
  StatementParser();
public:
  // Parse a statement
  // Returns whatever node was parsed
  ASTNode::Link statement();
};

class TokenParser: public StatementParser {
public:
  // See BlockParser's ctor
  TokenParser();
  
  void parse(std::vector<Token> input);
};

#endif
