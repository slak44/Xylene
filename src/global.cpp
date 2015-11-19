#include "global.hpp"

int
DEBUG_ENV,
EXPRESSION_STEPS,
PARSER_PRINT_INPUT,
PARSER_PRINT_TOKENS,
PARSER_PRINT_AS_EXPR,
PARSER_PRINT_DECL_TREE,
PARSER_PRINT_EXPR_TREE,
PARSER_PRINT_OPERATOR_TOKENS,
TOKEN_OPERATOR_PRINT_CONSTRUCTION,
TEST_INPUT;
std::string INPUT;

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
  getAndSetConstant(PARSER_PRINT_DECL_TREE, consts);
  getAndSetConstant(PARSER_PRINT_EXPR_TREE, consts);
  getAndSetConstant(PARSER_PRINT_OPERATOR_TOKENS, consts);
  getAndSetConstant(TOKEN_OPERATOR_PRINT_CONSTRUCTION, consts);
  getAndSetConstant(TEST_INPUT, consts);
  std::ifstream input("inputs.data");
  for (int i = 0; i < TEST_INPUT; i++) {
    std::string line; getline(input, line);
    if (i == TEST_INPUT - 1) INPUT = line;
  }
}

SyntaxError::SyntaxError(unsigned int lines):
  runtime_error("") {
  msg = "at line: " + std::to_string(lines);
}
SyntaxError::SyntaxError(std::string msg, unsigned int lines):
  runtime_error(msg.c_str()),
  msg(msg) {
  this->msg = msg + ", at line: " + std::to_string(lines);
}

std::string SyntaxError::toString() {
  return this->msg;
}

TypeError::TypeError():
  runtime_error("") {
}
TypeError::TypeError(std::string msg):
  runtime_error(msg.c_str()),
  msg(msg) {
}

std::string TypeError::toString() {
  return this->msg;
}
