#ifndef TOKEN_PARSER_HPP
#define TOKEN_PARSER_HPP

#include <string>
#include <vector>
#include <stack>
#include <algorithm>

#include "utils/util.hpp"
#include "utils/error.hpp"
#include "parser/baseParser.hpp"
#include "ast.hpp"
#include "token.hpp"
#include "operator.hpp"

/**
  \brief Parses a list of tokens, and produces an AST.
*/
class TokenParser: public BaseParser {
private:
  std::vector<Token> input;
  std::size_t pos = 0;
  
  /// Get token at current position
  inline Token current() {
    return input[pos];
  }
  /// Skip a number of tokens, usually just advances to the next one
  inline void skip(int skipped = 1) {
    pos += skipped;
    if (pos >= input.size()) pos = input.size() - 1;
  }
  /// Accept a TokenType
  inline bool accept(TokenType tok) {
    return tok == current().type;
  }
  /// Accept a Operator::Symbol
  inline bool accept(Operator::Symbol operatorSymbol) {
    if (!current().isOp()) return false;
    if (current().op().hasSymbol(operatorSymbol)) return true;
    return false;
  }
  /// Accept a Fixity
  inline bool accept(Fixity fixity) {
    return current().isOp() && current().op().hasFixity(fixity);
  }
  /// Accept only terminals
  inline bool acceptTerminal() {
    return current().isTerminal();
  }
  /// Accept anything that ends an expression
  inline bool acceptEndOfExpression() {
    return accept(TT::SEMI) || accept(TT::FILE_END) || accept(TT::DO);
  }
  /// Expect certain tokens to appear (throw otherwise)
  inline bool expect(TokenType tok, std::string errorMessage = "Unexpected symbol") {
    if (accept(tok)) {
      return true;
    } else {
      auto currentData = current().isOp() ? current().op().getSymbol() : current().data;
      throw Error("SyntaxError", errorMessage + " (found: " + currentData + ")", current().trace);
    }
  }
  /// Expect a semicolon \see expect
  inline void expectSemi() {
    expect(TT::SEMI, "Expected semicolon");
    skip();
  }
  
  // Expressions
  
  /// Creates an ExpressionNode from the current Token
  Node<ExpressionNode>::Link exprFromCurrent();
  /// Parse the stuff inside () or [] or calls
  Node<ExpressionNode>::Link parseCircumfixGroup(Token begin);
  /// Parse postfix ops in primaries
  Node<ExpressionNode>::Link parsePostfix(Node<ExpressionNode>::Link terminal);
  /**
    \brief Parse a primary in the expression
    
    Example of empty expressions: "" or ";"
    \returns an ExpressionNode, or nullptr if the expression is empty (terminates immediately)
  */
  Node<ExpressionNode>::Link parseExpressionPrimary();
  /// Implementation detail of expression, only exists to allow recursion.
  Node<ExpressionNode>::Link expressionImpl(Node<ExpressionNode>::Link lhs, int minPrecedence);
  /**
    \brief Parse an expression starting at the current token
    \param throwIfEmpty throws an error on empty expressions; if set to false, empty expressions return nullptr
  */
  Node<ExpressionNode>::Link expression(bool throwIfEmpty = true);
  
  // Declarations
  
  /// Creates a DeclarationNode from a list of types, handles initialization if available
  Node<DeclarationNode>::Link declarationFromTypes(TypeList typeList);
  /**
    \brief Parse a declaration starting at the current token
    \param throwIfEmpty throws an error on empty declarations; if set to false, empty declarations return nullptr
  */
  Node<DeclarationNode>::Link declaration(bool throwIfEmpty = true);

  // Blocks

  /**
    \brief Parse a block, depending on its type
    
    Normal code blocks are enclosed in 'do'/'end'.
    Root blocks aren't expected to be enclosed, and instead they end at the file end.
    If blocks can also be ended with 'else' besides 'end'.
  */
  Node<BlockNode>::Link block(BlockType type);
  
  // Functions
  
  /// Get a type list from current pos
  TypeList getTypeList();
  /**
    \brief Get arguments for signatures
    \returns an instance of FunctionSignature::Arguments
  */
  FunctionSignature::Arguments getSigArgs();
  /**
    \brief Parse a function starting at the current token
    \param isForeign if true, treat this as an external function declaration
  */
  Node<FunctionNode>::Link function(bool isForeign = false);

  // If statements
  
  /**
    \brief Parse an if statement starting at the current token
    
    This function assumes the 'if' keyword has been skipped.
  */
  Node<BranchNode>::Link ifStatement();
  
  // Type definitions
  
  Node<ConstructorNode>::Link constructor(Visibility, bool isForeign);
  Node<MethodNode>::Link method(Visibility, bool isStatic, bool isForeign);
  Node<MemberNode>::Link member(Visibility, bool isStatic);
  Node<TypeNode>::Link type();

  // Statements

  /**
    \brief Parse a statement
    \returns whatever node was parsed
  */
  ASTNode::Link statement();

public:  
  inline TokenParser() {}
  
  /**
    \brief Call this method to actually do the parsing before using getTree
    \param input list of tokens from lexer
  */
  TokenParser& parse(std::vector<Token> input);
};

#endif
