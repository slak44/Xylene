#ifndef OPERATOR_HPP
#define OPERATOR_HPP

#include <string>
#include <vector>
#include <unordered_map>

#include "utils/util.hpp"
#include "utils/error.hpp"

// The symbol used for the operator, eg '+', '=', '&&'
using OperatorSymbol = std::string;
// Convenience names from the operatorNameMap below
using OperatorName = std::string;
// Where in the operatorMap below it is
using OperatorIndex = int64;

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
  OperatorSymbol name;
  int precedence;
  Associativity associativity;
  Arity arity;
  Fixity fixity;
public:
  Operator(OperatorSymbol name, int precedence, Associativity associativity = ASSOCIATE_FROM_LEFT, Arity arity = BINARY, Fixity fixity = INFIX);
    
  OperatorSymbol getName() const;
  int getPrecedence() const;
  Associativity getAssociativity() const;
  Arity getArity() const;
  Fixity getFixity() const;
  
  bool operator==(const Operator& rhs) const;
  bool operator!=(const Operator& rhs) const;
};

const std::vector<Operator> operatorList {
  Operator("==", 7),
  Operator("!=", 7),
  
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
  Operator("|=", 1, ASSOCIATE_FROM_RIGHT),
  
  Operator(">>", 9),
  Operator("<<", 9),
  
  Operator("<=", 8),
  Operator("<", 8),
  Operator(">=", 8),
  Operator(">", 8),
  
  Operator("--", 13, ASSOCIATE_FROM_LEFT, UNARY, POSTFIX),
  Operator("++", 13, ASSOCIATE_FROM_LEFT, UNARY, POSTFIX),
  //  Operator("[]", 13), // TODO: find subscript better
  
  Operator(".", 13),
  
  Operator("--", 12, ASSOCIATE_FROM_RIGHT, UNARY, PREFIX),
  Operator("++", 12, ASSOCIATE_FROM_RIGHT, UNARY, PREFIX),
  Operator("-", 12, ASSOCIATE_FROM_RIGHT, UNARY, PREFIX),
  Operator("+", 12, ASSOCIATE_FROM_RIGHT, UNARY, PREFIX),
  Operator("~", 12, ASSOCIATE_FROM_RIGHT, UNARY, PREFIX),
  Operator("!", 12, ASSOCIATE_FROM_RIGHT, UNARY, PREFIX),
  
  Operator("*", 11),
  Operator("/", 11),
  Operator("%", 11),
  
  Operator("+", 10),
  Operator("-", 10),
  
  Operator("&&", 3), // Logical AND
  Operator("||", 2), // Logical OR
  
  Operator("&", 6), // Bitwise AND
  Operator("^", 5), // Bitwise XOR
  Operator("|", 4), // Bitwise OR
  
  Operator(",", 0)
};

static const std::unordered_map<OperatorName, OperatorIndex> operatorNameMap {
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

std::vector<char> getOperatorCharacters();
std::vector<char> getConstructCharacters();

// OperatorName -> Operator
const Operator& operatorFrom(const OperatorName& name);
// OperatorIndex -> OperatorName
OperatorName operatorNameFrom(OperatorIndex index);
// OperatorName -> OperatorIndex
OperatorIndex operatorIndexFrom(const OperatorName& name);

#endif
