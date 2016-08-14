#ifndef TOKEN_TYPE_HPP
#define TOKEN_TYPE_HPP

#include <iostream>
#include <string>
#include <unordered_map>

#include "utils/util.hpp"
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

std::string operator+(const char* a, const TokenType& tt);
std::string operator+(const TokenType& tt, const char* a);
std::string operator+(std::string& a, const TokenType& tt);
std::string operator+(const TokenType& tt, std::string& a);
std::ostream& operator<<(std::ostream& os, const TokenType& tt);

// Construct char -> TokenType
const std::unordered_map<char, TokenType> constructMap {
  {';', C_SEMI},
  {':', C_2POINT},
  {'(', C_PAREN_LEFT},
  {')', C_PAREN_RIGHT},
  {'[', C_SQPAREN_LEFT},
  {']', C_SQPAREN_RIGHT},
  {'?', C_QUESTION},
};

// Keyword text -> TokenType
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

// Pretty names for the rest of the TokenTypes
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

// string -> TokenType
TokenType getTokenTypeByName(std::string name);
// TokenType -> string
std::string getTokenTypeName(const TokenType& tt);

#endif
