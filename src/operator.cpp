#include "operator.hpp"

Operator::Operator(OperatorName name, int precedence, Associativity associativity, Arity arity, Fixity fixity):
  name(name), precedence(precedence), associativity(associativity), arity(arity), fixity(fixity) {
  if (arity == UNARY && fixity == INFIX) {
    throw InternalError("There are no unary infix operators", {METADATA_PAIRS});
  }
}
  
OperatorName Operator::getName() const {return name;}
int Operator::getPrecedence() const {return precedence;}
Associativity Operator::getAssociativity() const {return associativity;}
Arity Operator::getArity() const {return arity;}
Fixity Operator::getFixity() const {return fixity;}

bool Operator::operator==(const Operator& rhs) const {
  return name == rhs.name &&
    precedence == rhs.precedence &&
    associativity == rhs.associativity &&
    arity == rhs.arity &&
    fixity == rhs.fixity;
}
bool Operator::operator!=(const Operator& rhs) const {
  return !operator==(rhs);
}

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
  return operatorList[operatorNameMap.at(name)];
}

OperatorName operatorNameFrom(OperatorIndex index) {
  auto it = std::find_if(ALL(operatorNameMap), [=](auto mapPair) {
    return mapPair.second == index;
  });
  if (it == operatorNameMap.end()) throw InternalError("No such operator index", {METADATA_PAIRS, {"index", std::to_string(index)}});
  return it->first;
}

OperatorIndex operatorIndexFrom(const OperatorName& name) {
  return operatorNameMap.at(name);
}
