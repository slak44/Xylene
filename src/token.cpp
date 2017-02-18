#include "token.hpp"

std::string TokenType::toString() const {return prettyName;}
std::string TT::Keyword::getKeyword() const {return prettyName;}
char TT::Construct::getData() const {return data;}

TokenType TT::findConstruct(char c) {
  auto it = std::find_if(ALL(TT::constructs), [=](Construct t) {
    return c == t.getData();
  });
  if (it == std::end(TT::constructs)) return TT::UNPROCESSED;
  else return *it;
}

TokenType TT::findKeyword(std::string s) {
  auto it = std::find_if(ALL(TT::keywords), [=](Keyword t) {
    return s == t.getKeyword();
  });
  if (it == std::end(TT::keywords)) return TT::UNPROCESSED;
  else return *it;
}

TokenType TT::findByPrettyName(std::string prettyName) {
  auto it = std::find_if(ALL(TT::all), [=](TokenType t) {
    return prettyName == t.toString();
  });
  if (it == std::end(TT::all)) throw InternalError("Cannot determine TokenType", {
    METADATA_PAIRS,
    {"name", prettyName}
  });
  return *it;
}

Token::Token(TokenType type, std::string data, Trace trace):
  type(type), data(data), trace(trace) {}
Token::Token(TokenType type, Operator::Index idx, Trace trace):
  type(type), idx(idx), trace(trace) {}
Token::Token(TokenType type, Trace trace):
  type(type), trace(trace) {}

bool Token::isTerminal() const {
  return type == TT::IDENTIFIER ||
    type == TT::INTEGER ||
    type == TT::FLOAT ||
    type == TT::STRING ||
    type == TT::BOOLEAN;
}

bool Token::isOp() const {
  return type == TT::OPERATOR;
}

Operator Token::op() const {
  if (type != TT::OPERATOR) throw InternalError("Only available on operators", {
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
  return fmt::format("Token {0}, {1}, {2}", type, tokData, trace);
}
