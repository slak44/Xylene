#include "tokenType.hpp"

std::string operator+(const char* a, const TokenType& tt) {
  return a + getTokenTypeName(tt);
}
std::string operator+(const TokenType& tt, const char* a) {
  return getTokenTypeName(tt) + a;
}
std::string operator+(std::string& a, const TokenType& tt) {
  return operator+(a.c_str(), tt);
}
std::string operator+(const TokenType& tt, std::string& a) {
  return operator+(tt, a.c_str());
}
std::ostream& operator<<(std::ostream& os, const TokenType& tt) {
  return os << operator+("", tt);
}

TokenType getTokenTypeByName(std::string name) {
  try {
    return keywordsMap.at(name);
  } catch (std::out_of_range& oor) {}
  try {
    return constructMap.at(name[0]);
  } catch (std::out_of_range& oor) {}
  auto it = std::find_if(ALL(tokenTypeMap), [=](const std::pair<TokenType, std::string>& elem) {
    return elem.second == name;
  });
  if (it != tokenTypeMap.end()) return it->first;
  else throw InternalError("Cannot determine token type", {METADATA_PAIRS, {"name", name}});
}

std::string getTokenTypeName(const TokenType& tt) {
  try {
    // Try to use the map above
    return tokenTypeMap.at(tt);
  } catch (std::out_of_range& oor) {
    // Try to search in the keyword map
    auto keywordIt = std::find_if(ALL(keywordsMap), [&tt](const std::pair<std::string, TokenType>& pair) {
      return pair.second == tt;
    });
    if (keywordIt != keywordsMap.end()) return keywordIt->first;
  }
  // Give up and send the number if nothing else is found
  return std::to_string(tt);
}
