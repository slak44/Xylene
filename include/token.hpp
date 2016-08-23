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
private:
  inline void operatorCheck(ERR_METADATA_TYPES(file, func, line)) const {
    if (type != OPERATOR) throw InternalError("Only available on operators", {METADATA_PAIRS_FROM(file, func, line), {"token", this->toString()}});
  }
public:
  TokenType type; ///< \see TokenType
  std::string data = ""; ///< Stores the data that represents this token. May be processed
  Operator::Index idx = -1; ///< Only if the type is OPERATOR
  Trace trace; ///< At what line of the input was this Token encountered
  
  /**
    \brief Create a non-operator Token.
  */
  Token(TokenType type, std::string data, Trace trace);
  /**
    \brief Create an operator Token.
  */
  Token(TokenType type, Operator::Index idx, Trace trace);
  /**
    \brief Create a Token without initializing any data.
  */
  Token(TokenType type, Trace trace);
  
  /**
    \brief If this Token is terminal (can be processed by itself).
    
    Finds literals and identifiers basically.
  */
  inline bool isTerminal() const;
  /**
    \brief If this token contains an operator.
  */
  inline bool isOperator() const;
  
  /**
    \brief Get the stored operator.
    \throws InternalError if this Token does not contain an Operator
  */
  const Operator& getOperator() const;
  
  /**
    \brief Whether or not the stored Operator has a certain property.
    \throws InternalError if this Token does not contain an Operator
  */
  bool hasArity(Arity arity) const;
  /// \copydoc hasArity
  bool hasFixity(Fixity fixity) const;
  /// \copydoc hasArity
  bool hasAssociativity(Associativity asoc) const;
  /// \copydoc hasArity
  bool hasOperatorSymbol(Operator::Symbol name) const;
  
  /**
    \brief Get the precedence of the stored Operator.
    \throws InternalError if this Token does not contain an Operator
  */
  int getPrecedence() const;
  
  bool operator==(const Token& tok) const;
  bool operator!=(const Token& tok) const;
  
  std::string toString() const;
};

inline bool Token::isTerminal() const {
  return type == IDENTIFIER ||
    type == L_INTEGER ||
    type == L_FLOAT ||
    type == L_STRING ||
    type == L_BOOLEAN;
}

inline bool Token::isOperator() const {
  return idx >= 0;
}

inline std::ostream& operator<<(std::ostream& os, const Token& tok) {
  return os << tok.toString();
}

#endif
