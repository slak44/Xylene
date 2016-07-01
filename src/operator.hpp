#ifndef OPERATOR_HPP
#define OPERATOR_HPP

#include <string>
#include <vector>
#include <algorithm>
#include <functional>
#include <numeric>

#include "error.hpp"

enum Associativity: int {
  ASSOCIATE_FROM_LEFT,
  ASSOCIATE_FROM_RIGHT
};

enum Arity: int {
  NULLARY = 0,
  UNARY = 1,
  BINARY = 2,
  TERNARY = 3
};

enum Fixity: int {
  INFIX = 0, PREFIX = -1, POSTFIX = 1
};

class Operator {
private:
  std::string name;
  int precedence;
  Associativity associativity;
  Arity arity;
  Fixity fixity;
public:
  Operator(std::string name, int precedence, Associativity associativity = ASSOCIATE_FROM_LEFT, Arity arity = BINARY, Fixity fixity = INFIX):
    name(name), precedence(precedence), associativity(associativity), arity(arity), fixity(fixity) {
    if (arity == UNARY && fixity == INFIX) {
      throw InternalError("There are no unary infix operators", {METADATA_PAIRS});
    }
  }
    
  std::string getName() const {return name;}
  int getPrecedence() const {return precedence;}
  Associativity getAssociativity() const {return associativity;}
  Arity getArity() const {return arity;}
  Fixity getFixity() const {return fixity;}
  
  bool operator==(const Operator& rhs) const {
    return name == rhs.name &&
      precedence == rhs.precedence &&
      associativity == rhs.associativity &&
      arity == rhs.arity &&
      fixity == rhs.fixity;
  }
  bool operator!=(const Operator& rhs) const {
    return !operator==(rhs);
  }
};

const std::vector<Operator> operatorList {
  Operator("==", 7),
  Operator("!=", 7), // 1
  
  // TODO: Ternary "?:"
  Operator("=", 1, ASSOCIATE_FROM_RIGHT),
  Operator("+=", 1, ASSOCIATE_FROM_RIGHT),
  Operator("-=", 1, ASSOCIATE_FROM_RIGHT),
  Operator("*=", 1, ASSOCIATE_FROM_RIGHT),
  Operator("/=", 1, ASSOCIATE_FROM_RIGHT),
  Operator("%=", 1, ASSOCIATE_FROM_RIGHT),
  Operator("<<=", 1, ASSOCIATE_FROM_RIGHT),
  Operator(">>=", 1, ASSOCIATE_FROM_RIGHT),
  Operator("&=", 1, ASSOCIATE_FROM_RIGHT),
  Operator("^=", 1, ASSOCIATE_FROM_RIGHT),
  Operator("|=", 1, ASSOCIATE_FROM_RIGHT), // 12
  
  Operator(">>", 9),
  Operator("<<", 9), // 14
  
  Operator("<=", 8),
  Operator("<", 8),
  Operator(">=", 8),
  Operator(">", 8), // 18
  
  // Postfix
  Operator("--", 13, ASSOCIATE_FROM_LEFT, UNARY, POSTFIX),
  Operator("++", 13, ASSOCIATE_FROM_LEFT, UNARY, POSTFIX), // 20
  //  Operator("[]", 13), // TODO: find subscript better
  
  Operator(".", 13), // 21
  
  // Prefix
  Operator("--", 12, ASSOCIATE_FROM_RIGHT, UNARY, PREFIX),
  Operator("++", 12, ASSOCIATE_FROM_RIGHT, UNARY, PREFIX),
  Operator("-", 12, ASSOCIATE_FROM_RIGHT, UNARY, PREFIX),
  Operator("+", 12, ASSOCIATE_FROM_RIGHT, UNARY, PREFIX),
  Operator("~", 12, ASSOCIATE_FROM_RIGHT, UNARY, PREFIX), // Bitwise NOT
  Operator("!", 12, ASSOCIATE_FROM_RIGHT, UNARY, PREFIX), // Logical NOT
  // 27
  
  Operator("*", 11),
  Operator("/", 11),
  Operator("%", 11), // 30
  
  Operator("+", 10),
  Operator("-", 10), // 32
  
  Operator("&&", 3), // Logical AND
  Operator("||", 2), // Logical OR
  // 34
  
  Operator("&", 6), // Bitwise AND
  Operator("^", 5), // Bitwise XOR
  Operator("|", 4), // Bitwise OR
  // 37
  
  Operator(",", 0) // 38
};

typedef std::string OperatorName;

std::map<OperatorName, uint> operatorNameMap {
  {"Equality", 0},
  {"Inequality", 1},
  {"Assignment", 2},
  {"+Assignment", 3},
  {"-Assignment", 4},
  {"*Assignment", 5},
  {"/Assignment", 6},
  {"%Assignment", 7},
  {"<<Assignment", 8},
  {">>Assignment", 9},
  {"&Assignment", 10},
  {"^Assignment", 11},
  {"|Assignment", 12},
  {"Bitshift >>", 13},
  {"Bitshift <<", 14},
  {"Less or equal", 15},
  {"Less", 16},
  {"Greater or equal", 17},
  {"Greater", 18},
  {"Postfix --", 19},
  {"Postfix ++", 20},
  {"Member access", 21},
  {"Prefix --", 22},
  {"Prefix ++", 23},
  {"Unary -", 24},
  {"Unary +", 25},
  {"Bitwise NOT", 26},
  {"Logical NOT", 27},
  {"Multiply", 28},
  {"Divide", 29},
  {"Modulo", 30},
  {"Add", 31},
  {"Substract", 32},
  {"Logical AND", 33},
  {"Logical OR", 34},
  {"Bitwise AND", 35},
  {"Bitwise XOR", 36},
  {"Bitwise OR", 37},
  {"Comma", 38},
};

std::vector<char> getOperatorCharacters() {
  static std::vector<char> chars {};
  if (chars.size() == 0) {
    std::string mashed = std::accumulate(operatorList.begin(), operatorList.end(), std::string {},
    [](const std::string& previous, const Operator& op) {
      return previous + op.getName();
    });
    std::sort(mashed.begin(), mashed.end());
    mashed.erase(std::unique(mashed.begin(), mashed.end()), mashed.end());
    for (auto c : mashed) chars.push_back(c);
  }
  return chars;
}

std::vector<char> getConstructCharacters() {
  const static std::vector<char> chars {' ', '\n', '\0', '(', ')', '[', ']', '?', ';', ':'};
  return chars;
}

const Operator& operatorFrom(const OperatorName& name) {
  return operatorList[operatorNameMap[name]];
}

OperatorName operatorNameFrom(uint64 index) {
  auto it = std::find_if(ALL(operatorNameMap), [=](auto mapPair) {
    return mapPair.second == index;
  });
  if (it == operatorNameMap.end()) throw InternalError("No such operator index", {METADATA_PAIRS, {"index", std::to_string(index)}});
  return it->first;
}

#endif
