#ifndef TOKEN_HPP
#define TOKEN_HPP

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>

#include "utils/util.hpp"
#include "operator.hpp"
#include "utils/error.hpp"

/*
  L: literal
  K: keyword (2+ chars)
  C: construct (1 char)
*/
enum TokenType: int {
  L_INTEGER = 100, L_FLOAT, L_BOOLEAN, L_STRING,
  
  K_DEFINE = 200,
  K_FUNCTION, K_RETURN,
  K_DO, K_END,
  K_IF, K_ELSE, K_WHILE, K_FOR,
  K_FAT_ARROW, K_VOID,
  K_TYPE, K_CONSTR, K_PUBLIC, K_PRIVATE, K_PROTECT, K_STATIC,
  K_THROW, K_TRY, K_CATCH,
  
  C_SEMI = 300, C_2POINT, C_QUESTION,
  C_PAREN_LEFT, C_PAREN_RIGHT,
  C_SQPAREN_LEFT, C_SQPAREN_RIGHT,
  
  FILE_END = 999,
  OPERATOR = 1,
  IDENTIFIER = 5,
  UNPROCESSED = 0
};

const std::unordered_map<std::string, TokenType> keywordsMap {
  {"define", K_DEFINE},
  {"function", K_FUNCTION},
  {"return", K_RETURN},
  {"do", K_DO},
  {"end", K_END},
  {"if", K_IF},
  {"else", K_ELSE},
  {"while", K_WHILE},
  {"for", K_FOR},
  {"type", K_TYPE},
  {"constructor", K_CONSTR},
  {"public", K_PUBLIC},
  {"private", K_PRIVATE},
  {"protected", K_PROTECT},
  {"static", K_STATIC},
  {"throw", K_THROW},
  {"try", K_TRY},
  {"catch", K_CATCH},
  {"=>", K_FAT_ARROW},
  {"void", K_VOID}
};

const std::unordered_map<char, char> singleCharEscapeSeqences {
  {'a', '\a'},
  {'b', '\b'},
  {'f', '\f'},
  {'n', '\n'},
  {'r', '\r'},
  {'t', '\t'},
  {'v', '\v'},
  {'\\', '\\'},
  {'\'', '\''},
  {'"', '\"'},
  {'?', '\?'},
};

const std::unordered_map<char, TokenType> constructMap {
  {';', C_SEMI},
  {':', C_2POINT},
  {'(', C_PAREN_LEFT},
  {')', C_PAREN_RIGHT},
  {'[', C_SQPAREN_LEFT},
  {']', C_SQPAREN_RIGHT},
  {'?', C_QUESTION},
};

const static std::unordered_map<TokenType, std::string> tokenTypeMap {
  {L_INTEGER, "Integer"},
  {L_FLOAT, "Float"},
  {L_BOOLEAN, "Boolean"},
  {L_STRING, "String"},
  {C_SEMI, "Semicolon"},
  {C_2POINT, "Colon"},
  {C_QUESTION, "Question mark"},
  {C_PAREN_LEFT, "Left parenthesis"},
  {C_PAREN_RIGHT, "Right parenthesis"},
  {C_PAREN_LEFT, "Left square parenthesis"},
  {C_PAREN_RIGHT, "Right square parenthesis"},
  {FILE_END, "End of file"},
  {OPERATOR, "Operator"},
  {IDENTIFIER, "Identifier"},
  {UNPROCESSED, "Unprocessed?"}
};

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
  
  Token(TokenType type, std::string data, uint64 line): type(type), data(data), line(line) {}
  Token(TokenType type, size_t operatorIndex, uint64 line): type(type), operatorIndex(operatorIndex), line(line) {}
  Token(TokenType type, uint64 line): type(type), line(line) {}
  
  inline bool isTerminal() const {
    return type == IDENTIFIER ||
      type == L_INTEGER ||
      type == L_FLOAT ||
      type == L_STRING ||
      type == L_BOOLEAN;
  }
  
  inline bool isOperator() const {
    return operatorIndex >= 0;
  }
  
  const Operator& getOperator() const {
    operatorCheck(GET_ERR_METADATA);
    return operatorList[operatorIndex];
  }
  
  inline bool hasOperator(OperatorName name) {
    return operatorIndex == operatorNameMap[name];
  }
  
  bool hasArity(Arity arity) const {
    operatorCheck(GET_ERR_METADATA);
    return getOperator().getArity() == arity;
  }
  
  bool hasFixity(Fixity fixity) const {
    operatorCheck(GET_ERR_METADATA);
    return getOperator().getFixity() == fixity;
  }
  
  bool hasAssociativity(Associativity asoc) const {
    operatorCheck(GET_ERR_METADATA);
    return asoc == getOperator().getAssociativity();
  }
  
  bool hasOperatorName(std::string name) const {
    operatorCheck(GET_ERR_METADATA);
    return name == getOperator().getName();
  }
  
  int getPrecedence() const {
    operatorCheck(GET_ERR_METADATA);
    return getOperator().getPrecedence();
  }
  
  bool operator==(const Token& tok) const {
    return type == tok.type && data == tok.data && operatorIndex == tok.operatorIndex;
  }
  
  bool operator!=(const Token& tok) const {
    return !operator==(tok);
  }
  
  std::string toString() const {
    std::string data = !this->isOperator() ?
      (", data \"" + this->data + "\"") :
      (", operator " + this->getOperator().getName() + " (precedence " + std::to_string(this->getPrecedence()) + ")");
    return "Token " + this->type + data + ", at line " + std::to_string(this->line);
  }
};

inline std::ostream& operator<<(std::ostream& os, const Token& tok) {
  return os << tok.toString();
}

#endif
