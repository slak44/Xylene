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

/**
  \brief Base of TokenParser. Maintains all of its state, and provides convenience methods for manipulating it.
*/
class TokenBaseParser: public BaseParser {
protected:
  std::vector<Token> input;
  uint64 pos = 0;
  
  TokenBaseParser() {}
  virtual ~TokenBaseParser() = 0;
  
  /// Get token at current position
  inline Token current() {
    return input[pos];
  }
  /// Skip a number of tokens, usually just advances to the next one
  inline void skip(int by = 1) {
    pos += by;
  }
  /// Accept a TokenType
  inline bool accept(TokenType tok) {
    return tok == current().type;
  }
  /// Accept a Operator::Symbol
  inline bool accept(Operator::Symbol operatorSymbol) {
    if (!current().isOperator()) return false;
    if (current().hasOperatorSymbol(operatorSymbol)) return true;
    return false;
  }
  /// Accept a Fixity
  inline bool accept(Fixity fixity) {
    return current().isOperator() && current().hasFixity(fixity);
  }
  /// Accept only terminals
  inline bool acceptTerminal() {
    return current().isTerminal();
  }
  /// Accept anything that ends an expression
  inline bool acceptEndOfExpression() {
    return accept(C_SEMI) || accept(C_PAREN_RIGHT) || accept(FILE_END) || accept(K_DO);
  }
  /// Expect certain tokens to appear (throw otherwise)
  bool expect(TokenType tok, std::string errorMessage = "Unexpected symbol");
  /// Expect a semicolon \see expect
  void expectSemi();
};

/**
  \brief Provides the \link expression \endlink method for parsing an expression.
*/
class ExpressionParser: virtual public TokenBaseParser {
protected:
  ExpressionParser() {}
private:
  /// Creates an ExpressionNode from the current Token
  Node<ExpressionNode>::Link exprFromCurrent();
  /**
    \brief Parse a primary in the expression
    
    ';' is an empty expression for example
    \returns an ExpressionNode, or nullptr if the expression is empty (terminates immediately)
  */
  Node<ExpressionNode>::Link parseExpressionPrimary();
  /**
    \brief Implementation detail of expression
    
    This method exists to allow recursion.
  */
  Node<ExpressionNode>::Link expressionImpl(Node<ExpressionNode>::Link lhs, int minPrecedence);
public:
  /**
    \brief Parse an expression starting at the current token
    \param throwIfEmpty throws an error on empty expressions; if set to false, empty expressions return nullptr
  */
  Node<ExpressionNode>::Link expression(bool throwIfEmpty = true);
};

/**
  \brief Provides the \link declaration \endlink method for parsing a declaration.
  
  Depends on ExpressionParser for expression parsing.
*/
class DeclarationParser: virtual public ExpressionParser {
protected:
  DeclarationParser() {}
private:
  /// Creates a DeclarationNode from a list of types, handles initialization if available
  Node<DeclarationNode>::Link declarationFromTypes(TypeList typeList);
public:
  /**
    \brief Parse a declaration starting at the current token
    \param throwIfEmpty throws an error on empty declarations; if set to false, empty declarations return nullptr
  */
  Node<DeclarationNode>::Link declaration(bool throwIfEmpty = true);
};

class StatementParser;

/**
  \brief Provides the \link block \endlink method for parsing a block.
  
  Depends on StatementParser for parsing individual statements.
*/
class BlockParser: virtual public TokenBaseParser {
protected:
  /// Storing this pointer avoids circular inheritance between BlockParser and StatementParser
  StatementParser* stp;
  /// \copydoc stp
  BlockParser(StatementParser* stp);
public:
  /**
    \brief Parse a block, depending on its type
    
    Normal code blocks are enclosed in K_DO/K_END.
    Root blocks aren't expected to be enclosed, and instead they end at the file end.
    If blocks can also be ended with K_ELSE besides K_END.
  */
  Node<BlockNode>::Link block(BlockType type);
};

/**
  \brief Provides the \link ifStatement \endlink method for parsing an if statement.
  
  Depends on ExpressionParser for parsing expressions.
  Depends on BlockParser for parsing blocks.
*/
class IfStatementParser: virtual public ExpressionParser, virtual public BlockParser {
protected:
  /// \copydoc BlockNode(StatementParser*)
  IfStatementParser(StatementParser* stp);
public:
  /**
    \brief Parse an if statement starting at the current token
    
    This function assumes K_IF has been skipped.
  */
  Node<BranchNode>::Link ifStatement();
};


/**
  \brief Provides the \link statement \endlink method for parsing an a statement.
  
  Depends on ExpressionParser for parsing expressions.
  Depends on IfStatementParser for parsing if statements.
  Depends on BlockParser for parsing blocks.
  Depends on DeclarationParser for parsing declarations.
*/
class StatementParser: virtual public IfStatementParser, virtual public DeclarationParser {
protected:
  /// \see BlockNode(StatementParser*)
  StatementParser();
public:
  /**
    \brief Parse a statement
    
    \returns whatever node was parsed
  */
  ASTNode::Link statement();
};

/**
  \brief Parses a list of tokens, and produces an AST.
*/
class TokenParser: public StatementParser {
public:
  TokenParser();
  
  /**
    \brief Call this method to actually do the parsing before using getTree
    \param input list of tokens from lexer
  */
  void parse(std::vector<Token> input);
};

#endif
