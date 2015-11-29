#ifndef TOKENS_H_
#define TOKENS_H_

#include <string>
#include <vector>

#include "global.hpp"
#include "operators.hpp"
#include "builtins.hpp"

namespace lang {
  
  enum TokenType: int {
    INTEGER, FLOAT, BOOLEAN, STRING, ARRAY, // Literals
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
    * For OPERATOR: Operator*
    * For INTEGER: Integer*
    * For FLOAT: Float*
    * For VARIABLE: Variable*
    */
    void* typeData = nullptr;
    /*
    * Location of Token in code.
    * -1 by default. -2 when Token was created by interpreter.
    */
    int line = -1;
    
    Token();
    Token(std::string data, TokenType type, int line);
    Token(Operator* opContent, TokenType type, int line);
    Token(Object* obj, TokenType type, int line);
  };
  
  inline std::ostream& operator<<(std::ostream& os, Token& tok) { 
    return os << "Token " << tok.data << ", TokenType " << tok.type << ", with data pointer " << tok.typeData << ", at line " << tok.line;
  }
  
  extern std::function<bool(Token)> isNewLine;
  
} /* namespace lang */

#endif /* TOKENS_H_ */
