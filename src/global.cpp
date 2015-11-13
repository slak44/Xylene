#include "global.hpp"

int
DEBUG_ENV,
EXPRESSION_STEPS,
PARSER_PRINT_INPUT,
PARSER_PRINT_TOKENS,
PARSER_PRINT_AS_EXPR,
PARSER_PRINT_OPERATOR_TOKENS,
TOKEN_OPERATOR_PRINT_CONSTRUCTION,
TEST_INPUT;

void getAndSetConstant(int& constRef, std::ifstream& is) {
  std::string line;
  getline(is, line);
  constRef = line[0] - '0';
}

void getConstants() {
  std::ifstream consts("constants.data");
  getAndSetConstant(DEBUG_ENV, consts);
  getAndSetConstant(EXPRESSION_STEPS, consts);
  getAndSetConstant(PARSER_PRINT_INPUT, consts);
  getAndSetConstant(PARSER_PRINT_TOKENS, consts);
  getAndSetConstant(PARSER_PRINT_AS_EXPR, consts);
  getAndSetConstant(PARSER_PRINT_OPERATOR_TOKENS, consts);
  getAndSetConstant(TOKEN_OPERATOR_PRINT_CONSTRUCTION, consts);
  getAndSetConstant(TEST_INPUT, consts);
}

SyntaxError::SyntaxError(unsigned int lines):runtime_error("") {
  msg = "at line: " + std::to_string(lines);
}
SyntaxError::SyntaxError(std::string msg, unsigned int lines):runtime_error(msg.c_str()) {
  this->msg = msg + ", at line: " + std::to_string(lines);
}

std::string SyntaxError::getMessage() {
  return msg;
}
