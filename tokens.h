#ifndef TOKENS_H_
#define TOKENS_H_

#include <string>
#include <vector>

#include "global.h"
#include "operators.h"
#include "builtins.h"

enum TokenType: int {
  INTEGER, FLOAT, STRING, ARRAY, // Literals
  OPERATOR, KEYWORD, CONSTRUCT, // Language parts
  TYPE, VARIABLE, FUNCTION, // Defined structures
  UNPROCESSED
};

class Token {
public:
  std::string data = "";
  TokenType type = UNPROCESSED;
  /*
   * Depending on the TokenType, the pointer has the following possible types:
   * For OPERATOR: ops::Operator*
   * For INTEGER: builtins::Integer*
   */
  void* typeData = nullptr;
  // Location of Token
  int line = -1;
  
  Token();
  Token(std::string data, TokenType type, int line);
  Token(ops::Operator& opContent, TokenType type, int line);
  Token(builtins::Object& obj, TokenType type, int line);
};

inline std::ostream& operator<<(std::ostream& os, Token& tok) { 
  return os << "Token " << tok.data << ", TokenType " << tok.type << ", at line " << tok.line;
}

#endif /* TOKENS_H_ */
