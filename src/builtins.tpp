template<typename... Args>
void concatenateNames(std::string& result, Object*& obj, Args&... args) {
  if (obj->getTypeData() == "Variable") obj = dynamic_cast<Variable*>(obj)->read();
  result += obj->getTypeData() + " ";
  concatenateNames(result, args...);
}

// TODO: check for nullptr in any of the arguments, throw SyntaxError. Get line numbers from higher up, maybe default param
template<typename... Args>
Object* runOperator(Operator* op, Args... pr) {
  std::string funSig = "";
  if (op->toString().back() == '=') funSig = "Variable Object";
  else concatenateNames(funSig, pr...);
  // 1. Get a boost::any instance from the OperatorMap
  // 2. Use boost::any_cast to get a pointer to the operator function
  // 3. Dereference pointer and call function
  try {
    auto result = (*boost::any_cast<std::function<Object*(Args...)>*>(opsMap[*op][funSig]))(pr...);
    return result;
  } catch (boost::bad_any_cast& bac) {
    throw TypeError("Cannot find operation for operator '" + op->toString() + "' and operands '" + funSig + "'.\n");
  }
}