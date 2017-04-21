#include "llvm/compiler.hpp"

ValueWrapper::Link ModuleCompiler::load(ValueWrapper::Link maybePointer) {
  if (llvm::dyn_cast_or_null<llvm::PointerType>(maybePointer->val->getType())) {
    return std::make_shared<ValueWrapper>(
      builder->CreateLoad(maybePointer->val, "loadIfPointer"),
      maybePointer->ty
    );
  }
  return maybePointer;
}

bool ModuleCompiler::operatorTypeCheck(
  std::vector<ValueWrapper::Link> checkThis,
  std::vector<std::vector<AbstractId::Link>> checkAgainst
) {
  for (auto entry : checkAgainst) {
    if (entry.size() != checkThis.size()) continue;
    for (std::size_t i = 0; i < entry.size(); i++) {
      if (entry[i] != checkThis[i]->ty) goto continueFor;
    }
    return true;
    continueFor:;
  }
  return false;
}
