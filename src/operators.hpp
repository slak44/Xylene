#ifndef OPERATORS_H_
#define OPERATORS_H_

#include <string>
#include <vector>
#include <algorithm>
#include "global.hpp"

namespace lang {

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
  
  // TODO: Do something about this duplication
  std::string getName() const;
  Arity getArity() const;
  Associativity getAssociativity() const;
  int getPrecedence() const;
  
  std::string getName();
  Arity getArity();
  Associativity getAssociativity();
  int getPrecedence();

  bool operator==(const Operator& right) const;
  bool operator!=(const Operator& right) const;
};

std::vector<char> getOperatorCharacters();
bool isReservedChar(char& c);

struct OperatorHash {
  std::size_t operator()(const Operator& k) const {
    return std::hash<std::string>()(k.getName()) + k.getPrecedence() + k.getArity() + k.getAssociativity();
  }
};

extern std::vector<Operator> opList;

inline std::ostream& operator<<(std::ostream& os, Operator& op) { 
  return os << op.getName() << ", with precedence " << op.getPrecedence() << ", associativity " << op.getAssociativity() << ", arity " << op.getArity();
}

} /* namespace lang */

#endif /* OPERATORS_H_ */
