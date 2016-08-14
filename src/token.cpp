#include "token.hpp"

Token::Token(TokenType type, std::string data, uint64 line): type(type), data(data), line(line) {}
Token::Token(TokenType type, size_t operatorIndex, uint64 line): type(type), operatorIndex(operatorIndex), line(line) {}
Token::Token(TokenType type, uint64 line): type(type), line(line) {}

inline bool Token::isTerminal() const {
  return type == IDENTIFIER ||
    type == L_INTEGER ||
    type == L_FLOAT ||
    type == L_STRING ||
    type == L_BOOLEAN;
}

inline bool Token::isOperator() const {
  return operatorIndex >= 0;
}

const Operator& Token::getOperator() const {
  operatorCheck(GET_ERR_METADATA);
  return operatorList[operatorIndex];
}

bool Token::hasArity(Arity arity) const {
  operatorCheck(GET_ERR_METADATA);
  return getOperator().getArity() == arity;
}

bool Token::hasFixity(Fixity fixity) const {
  operatorCheck(GET_ERR_METADATA);
  return getOperator().getFixity() == fixity;
}

bool Token::hasAssociativity(Associativity asoc) const {
  operatorCheck(GET_ERR_METADATA);
  return asoc == getOperator().getAssociativity();
}

bool Token::hasOperatorName(std::string name) const {
  operatorCheck(GET_ERR_METADATA);
  return name == getOperator().getName();
}

int Token::getPrecedence() const {
  operatorCheck(GET_ERR_METADATA);
  return getOperator().getPrecedence();
}

bool Token::operator==(const Token& tok) const {
  return type == tok.type && data == tok.data && operatorIndex == tok.operatorIndex;
}

bool Token::operator!=(const Token& tok) const {
  return !operator==(tok);
}

std::string Token::toString() const {
  std::string data = !this->isOperator() ?
    (", data \"" + this->data + "\"") :
    (", operator " + this->getOperator().getName() + " (precedence " + std::to_string(this->getPrecedence()) + ")");
  return "Token " + this->type + data + ", at line " + std::to_string(this->line);
}

inline std::ostream& operator<<(std::ostream& os, const Token& tok) {
  return os << tok.toString();
}
