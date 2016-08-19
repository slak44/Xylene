#ifndef TOKEN_TYPE_HPP
#define TOKEN_TYPE_HPP

#include <iostream>
#include <string>
#include <unordered_map>

#include "utils/util.hpp"
#include "utils/error.hpp"

/**
  \brief Possible types for Token.
  
  L: literal\n
  K: keyword (2+ chars)\n
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

/// Overload for string concatenation
std::string operator+(const char* a, const TokenType& tt);
/// \copydoc operator+(const char*,const TokenType&)
std::string operator+(const TokenType& tt, const char* a);
/// \copydoc operator+(const char*,const TokenType&)
std::string operator+(std::string& a, const TokenType& tt);
/// \copydoc operator+(const char*,const TokenType&)
std::string operator+(const TokenType& tt, std::string& a);
/**
  \brief Allow TokenType to be printed using its pretty name.
  
  \sa getTokenTypeName
*/
std::ostream& operator<<(std::ostream& os, const TokenType& tt);

/**
  \brief Maps a construct's character to its respective TokenType.
*/
const std::unordered_map<char, TokenType> constructMap {
  {';', C_SEMI},
  {':', C_2POINT},
  {'(', C_PAREN_LEFT},
  {')', C_PAREN_RIGHT},
  {'[', C_SQPAREN_LEFT},
  {']', C_SQPAREN_RIGHT},
  {'?', C_QUESTION},
};

/**
  \brief Maps a keyword's text to its respective TokenType.
*/
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

/**
  \brief Maps the second character in an escape sequence (eg the 'n' in \\n) to the actual escaped code.
*/
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

/**
  \brief Pretty names for the TokenTypes that don't already have some.
*/
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

/**
  \brief Attempt to get a TokenType from its pretty name.

  Example results:\n
    Boolean -> L_BOOLEAN\n
    if -> K_IF\n
    ; -> C_SEMI\n
    
  Reverse operation of getTokenTypeName.
  \sa getTokenTypeName
*/
TokenType getTokenTypeByName(const std::string& name);
/**
  \brief Attempt to get a pretty name from a TokenType.
  
  Reverse operation of getTokenTypeByName.
  \sa getTokenTypeByName
*/
std::string getTokenTypeName(TokenType tt);

#endif
