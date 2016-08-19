#include "operator.hpp"

Operator::Operator(Operator::Name name, int precedence, Associativity associativity, Arity arity, Fixity fixity, RequireReferenceList refList):
  name(name), precedence(precedence), associativity(associativity), arity(arity), fixity(fixity), refList(refList) {
  if (arity == UNARY && fixity == INFIX) {
    throw InternalError("There are no unary infix operators", {METADATA_PAIRS});
  }
}
  
Operator::Name Operator::getName() const {return name;}
int Operator::getPrecedence() const {return precedence;}
Associativity Operator::getAssociativity() const {return associativity;}
Arity Operator::getArity() const {return arity;}
Fixity Operator::getFixity() const {return fixity;}
Operator::RequireReferenceList Operator::getRefList() const {return refList;}

bool Operator::operator==(const Operator& rhs) const {
  return name == rhs.name &&
    precedence == rhs.precedence &&
    associativity == rhs.associativity &&
    arity == rhs.arity &&
    fixity == rhs.fixity &&
    refList == rhs.refList;
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

const Operator& operatorFrom(const Operator::Name& name) {
  return operatorList[operatorNameMap.at(name)];
}

Operator::Name operatorNameFrom(Operator::Index index) {
  auto it = std::find_if(ALL(operatorNameMap), [=](auto mapPair) {
    return mapPair.second == index;
  });
  if (it == operatorNameMap.end()) throw InternalError("No such operator index", {METADATA_PAIRS, {"index", std::to_string(index)}});
  return it->first;
}

Operator::Index operatorIndexFrom(const Operator::Name& name) {
  return operatorNameMap.at(name);
}
