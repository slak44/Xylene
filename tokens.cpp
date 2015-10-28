#include "tokens.h"

Token::Token() {}
Token::Token(std::string data, TokenType type, int line):
  data(data),
  type(type),
  line(line) {}

Token::Token(ops::Operator& opContent, TokenType type, int line):
  data(opContent.getName()),
  type(type),
  typeData(&opContent),
  line(line) {
  if (TOKEN_OPERATOR_PRINT_CONSTRUCTION) print("Initialized Token with Operator: ", opContent, ", at address ", &opContent, "\n");
}
