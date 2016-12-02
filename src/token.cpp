#include "token.hpp"

Token::Token(TokenType type, std::string data, Trace trace):
  type(type), data(data), trace(trace) {}
Token::Token(TokenType type, Operator::Index idx, Trace trace):
  type(type), idx(idx), trace(trace) {}
Token::Token(TokenType type, Trace trace):
  type(type), trace(trace) {}

bool Token::isTerminal() const {
  return type == IDENTIFIER ||
    type == L_INTEGER ||
    type == L_FLOAT ||
    type == L_STRING ||
    type == L_BOOLEAN;
}

bool Token::isOp() const {
  return type == OPERATOR;
}

Operator Token::op() const {
  if (type != OPERATOR) throw InternalError("Only available on operators", {
    METADATA_PAIRS,
    {"token", this->toString()}
  });
  return Operator::list[idx];
}

bool Token::operator==(const Token& tok) const {
  return type == tok.type && data == tok.data && idx == tok.idx;
}
bool Token::operator!=(const Token& tok) const {
  return !operator==(tok);
}

std::string Token::toString() const {
  std::string tokData = isOp() ?
    "operator " + op().getName() :
    "data \"" + data + "\"";
  return "Token " + type + ", " + tokData + ", " + trace.toString();
}
