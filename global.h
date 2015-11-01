#ifndef GLOBAL_H_
#define GLOBAL_H_

extern int
DEBUG_ENV,
EXPRESSION_STEPS,
PARSER_PRINT_TOKENS,
PARSER_PRINT_AS_EXPR,
PARSER_PRINT_OPERATOR_TOKENS,
TOKEN_OPERATOR_PRINT_CONSTRUCTION,
TEST_INPUT;

#include <fstream>
#include <iostream>
#include <stdexcept>
#include "std_is_missing_stuff.h"

void getConstants();

template<typename T>
void print(T thing) {
  std::cout << thing;
}

template<typename T, typename... Args>
void print(T thing, Args... args) {
  print(thing);
  print(args...);
}

class SyntaxError: std::runtime_error {
private:
  std::string msg;
public:
  SyntaxError(unsigned int lines);
  SyntaxError(std::string msg, unsigned int lines);

  std::string getMessage();
};

#endif /* GLOBAL_H_ */
