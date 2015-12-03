template<typename... Args>
void concatenateNames(std::string& result, unsigned int line, Object*& obj, Args&... args) {
  if (obj == nullptr) throw TypeError("Variable is empty or has not been defined", line);
  while (obj->getTypeData() == "Variable") {
    obj = dynamic_cast<Variable*>(obj)->read();
    if (obj == nullptr) throw TypeError("Variable is empty or has not been defined", line);
  }
  result += obj->getTypeData() + " ";
  concatenateNames(result, line, args...);
}

// TODO: operators with side effects(eg ++ --) do not update the variable they're being pulled from
template<typename... Args>
Object* runOperator(Operator* op, unsigned int line, Args... pr) {
  std::string funSig = "";
  if (op->toString().back() == '=' && op->getPrecedence() == 1) funSig = "Variable Object";
  else concatenateNames(funSig, line, pr...);
  // 1. Get a boost::any instance from the OperatorMap
  // 2. Use boost::any_cast to get a pointer to the operator function
  // 3. Dereference pointer and call function
  try {
    auto result = (*boost::any_cast<std::function<Object*(Args...)>*>(opsMap[*op][funSig]))(pr...);
    return result;
  } catch (boost::bad_any_cast& bac) {
    throw TypeError("Cannot find operation for operator '" + op->toString() + "' and operands '" + funSig + "'", line);
  }
}