#ifndef GLOBAL_H_
#define GLOBAL_H_

typedef long double double64;
typedef long long int int64;
typedef unsigned long long int uint64;

#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <functional>
#include <algorithm>
#include <type_traits>

extern int
DEBUG_ENV,
EXPRESSION_STEPS,
PARSER_PRINT_INPUT,
PARSER_PRINT_TOKENS,
PARSER_PRINT_AS_EXPR,
PARSER_PRINT_DECL_TREE,
PARSER_PRINT_COND_TREE,
PARSER_PRINT_EXPR_TREE,
PARSER_PRINT_OPERATOR_TOKENS,
TOKEN_OPERATOR_PRINT_CONSTRUCTION,
TEST_INPUT;
extern std::string INPUT;

void getConstants(char* arg);

template<typename T>
std::size_t hash(T element);
template<typename T>
std::size_t hash(std::vector<T>& element);
template<typename T, typename... HashTypes>
std::size_t hash(std::size_t init, T element, HashTypes... ht);
template<typename T, typename... HashTypes>
std::size_t hash(T element, HashTypes... ht);
template<typename T>
std::size_t hashV(std::vector<T>& element);

template<typename T, typename Lambda>
std::vector<std::vector<T> > splitVector(std::vector<T> origin, Lambda& shouldSplit);

template<typename T>
std::ostream& operator<<(std::ostream& os, std::vector<T>& vec) {
  for (unsigned int i = 0; i < vec.size(); ++i) os << vec[i] << std::endl;
  return os;
}

template <typename T>
bool contains(T element, std::vector<T> vec);

template<typename T>
void print(T thing);
template<typename T, typename... Args>
void print(T thing, Args... args);

#include "global.tpp"

class Stringifyable {
public:
  virtual std::string toString() = 0;
};

class SyntaxError: std::runtime_error, Stringifyable {
private:
  std::string msg;
public:
  SyntaxError(unsigned int lines);
  SyntaxError(std::string msg, unsigned int lines);
  
  std::string toString();
};

class TypeError: std::runtime_error, Stringifyable {
private:
  std::string msg;
public:
  TypeError();
  TypeError(std::string msg);
  
  std::string toString();
};

#endif /* GLOBAL_H_ */
