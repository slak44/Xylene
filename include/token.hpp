#ifndef TOKEN_HPP
#define TOKEN_HPP

#include <string>

#include "tokenType.hpp"
#include "operator.hpp"

struct Token {
private:
  inline void operatorCheck(ERR_METADATA_TYPES(file, func, line)) const {
    if (type != OPERATOR) throw InternalError("Only available on operators", {METADATA_PAIRS_FROM(file, func, line), {"token", this->toString()}});
  }
public:
  TokenType type;
  std::string data = "";
  int operatorIndex = -1; // Index in the operator list
  uint64 line = 0;
  
  Token(TokenType type, std::string data, uint64 line);
  Token(TokenType type, size_t operatorIndex, uint64 line);
  Token(TokenType type, uint64 line);
  
  inline bool isTerminal() const;
  inline bool isOperator() const;
  
  const Operator& getOperator() const;
  
  bool hasArity(Arity arity) const;
  bool hasFixity(Fixity fixity) const;
  bool hasAssociativity(Associativity asoc) const;
  bool hasOperatorName(std::string name) const;
  
  int getPrecedence() const;
  
  bool operator==(const Token& tok) const;
  bool operator!=(const Token& tok) const;
  
  std::string toString() const;
};

inline std::ostream& operator<<(std::ostream& os, const Token& tok);

#endif
