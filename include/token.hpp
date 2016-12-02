#ifndef TOKEN_HPP
#define TOKEN_HPP

#include <string>

#include "utils/util.hpp"
#include "utils/trace.hpp"
#include "utils/error.hpp"
#include "operator.hpp"
#include "tokenType.hpp"

/**
  \brief Holds a token encountered by the lexer.
*/
class Token {
public:
  TokenType type; ///< \see TokenType
  std::string data = ""; ///< Stores the data that represents this token. May be processed
  Operator::Index idx = -1; ///< Only if the type is OPERATOR
  Trace trace; ///< At what line of the input was this Token encountered
  
  /// Create a non-operator Token.
  Token(TokenType type, std::string data, Trace trace);
  /// Create an operator Token.
  Token(TokenType type, Operator::Index idx, Trace trace);
  /// Create a Token without initializing any data.
  Token(TokenType type, Trace trace);
  
  /// Matches identifiers and literals
  bool isTerminal() const;
  /// Matches operators
  bool isOp() const;
  
  /// Get the stored operator
  Operator op() const;
    
  bool operator==(const Token& tok) const;
  bool operator!=(const Token& tok) const;
  
  std::string toString() const;
};

inline std::ostream& operator<<(std::ostream& os, const Token& tok) {
  return os << tok.toString();
}

#endif
