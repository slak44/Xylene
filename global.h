#ifndef GLOBAL_H_
#define GLOBAL_H_

#define DEBUG_ENV true
#define IF_DEBUGGING(x) DEBUG_ENV && x

#define EXPRESSION_STEPS                  IF_DEBUGGING(false)
#define PARSER_PRINT_TOKENS               IF_DEBUGGING(false)
#define PARSER_PRINT_AS_EXPR              IF_DEBUGGING(true)
#define PARSER_PRINT_OPERATOR_TOKENS      IF_DEBUGGING(false)
#define TOKEN_OPERATOR_PRINT_CONSTRUCTION IF_DEBUGGING(false)
#define TEST_INPUT                        7

#include <iostream>
#include <stdexcept>
#include "std_is_missing_stuff.h"

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
  SyntaxError(unsigned int lines):runtime_error("") {
    msg = "at line: " + to_string(lines);
  }
  SyntaxError(std::string msg, unsigned int lines):runtime_error(msg.c_str()) {
    this->msg = msg + ", at line: " + to_string(lines);
  }

  std::string getMessage() {
    return msg;
  }
};

#endif /* GLOBAL_H_ */
