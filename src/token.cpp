#include "token.hpp"

TokenType TT::findConstruct(char c) noexcept {
  auto it = std::find_if(ALL(TT::constructs), [=](Construct t) {
    return c == t.getData();
  });
  if (it == std::end(TT::constructs)) return TT::UNPROCESSED;
  else return *it;
}

TokenType TT::findKeyword(std::string s) noexcept {
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

Operator Token::op() const {
  if (type != TT::OPERATOR) throw InternalError("Only available on operators", {
    METADATA_PAIRS,
    {"token", this->toString()}
  });
  return Operator::list[idx];
}

std::string Token::toString() const noexcept {
  std::string tokData = isOp() ?
    "operator " + op().getName() :
    "data \"" + data + "\"";
  return fmt::format("Token {0}, {1}, {2}", type, tokData, trace);
}
