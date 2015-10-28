#ifndef OPERATORS_H_
#define OPERATORS_H_

#include <string>
#include <vector>
#include <algorithm>
#include "global.h"

namespace ops {

enum Associativity: int {
  ASSOCIATE_FROM_LEFT,
  ASSOCIATE_FROM_RIGHT
};

enum Arity: int {
  UNARY = 1,
  BINARY = 2,
  TERNARY = 3
};

class Operator {
private:
  std::string op;
  int precedence = -1;
  Associativity asc = ASSOCIATE_FROM_LEFT;
  Arity ar = BINARY;
public:
  Operator(std::string opName, int precedence, Associativity asc, Arity ar);
  Operator(std::string opName, int precedence, Arity ar = BINARY);
  
  std::string getName();
  Arity getArity();
  Associativity getAssociativity();
  int getPrecedence();

  bool operator==(const Operator& right);
  bool operator!=(const Operator& right);
  bool operator< (const Operator& right);
  bool operator> (const Operator& right);
  bool operator>=(const Operator& right);
  bool operator<=(const Operator& right);
};

std::vector<char> getOperatorCharacters();
bool isReservedChar(char& c);

extern std::vector<Operator> opList;

inline std::ostream& operator<<(std::ostream& os, Operator& op) { 
  return os << op.getName() << ", with precedence " << op.getPrecedence() << ", associativity " << op.getAssociativity() << ", arity " << op.getArity();
}

}
#endif /* OPERATORS_H_ */
