#include "runtime/runtime.hpp"

void _xyl_finish(const char* message, int exitCode) {
  println(message);
  std::exit(exitCode);
}

void _xyl_typeErrIfIncompatible(_xyl_Value* val, _xyl_Value* newVal) {
  bool compat = _xyl_checkTypeCompat(val, newVal);
  if (!compat) {
    std::stringstream err;
    err << "TypeError: Incompatible types";
    err << " \"" << _xyl_typeOf(val) << "\" and \"" << _xyl_typeOf(newVal) << "\"";
    _xyl_finish(err.str().c_str(), -1);
  }
}

bool _xyl_checkTypeCompat(_xyl_Value* val, _xyl_Value* newVal) {
  // TODO
  UNUSED(val);
  UNUSED(newVal);
  return true;
}

const char* _xyl_typeOf(_xyl_Value* val) {
  // TODO
  return std::to_string(val->currentType).c_str();
}
