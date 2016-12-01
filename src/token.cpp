#include "token.hpp"

Token::Token(TokenType type, std::string data, Trace trace): type(type), data(data), trace(trace) {}
Token::Token(TokenType type, Operator::Index idx, Trace trace): type(type), idx(idx), trace(trace) {}
Token::Token(TokenType type, Trace trace): type(type), trace(trace) {}

const Operator& Token::getOperator() const {
  operatorCheck(GET_ERR_METADATA);
  return Operator::list[idx];
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
  return "Token " + this->type + ", " + data + ", " + trace.toString();
}
