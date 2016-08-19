#include "token.hpp"

Token::Token(TokenType type, std::string data, uint64 line): type(type), data(data), line(line) {}
Token::Token(TokenType type, Operator::Index idx, uint64 line): type(type), idx(idx), line(line) {}
Token::Token(TokenType type, uint64 line): type(type), line(line) {}

const Operator& Token::getOperator() const {
  operatorCheck(GET_ERR_METADATA);
  return operatorList[idx];
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

bool Token::hasOperatorSymbol(Operator::Symbol name) const {
  operatorCheck(GET_ERR_METADATA);
  return name == getOperator().getName();
}

int Token::getPrecedence() const {
  operatorCheck(GET_ERR_METADATA);
  return getOperator().getPrecedence();
}

bool Token::operator==(const Token& tok) const {
  return type == tok.type && data == tok.data && idx == tok.idx;
}

bool Token::operator!=(const Token& tok) const {
  return !operator==(tok);
}

std::string Token::toString() const {
  std::string data = !this->isOperator() ?
    ("data \"" + this->data + "\"") :
    ("operator " + this->getOperator().getName() + " (precedence " + std::to_string(this->getPrecedence()) + ")");
  return "Token " + this->type + ", " + data + ", at line " + std::to_string(this->line);
}
