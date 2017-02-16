#include "llvm/runner.hpp"

Runner::Runner(ModuleCompiler::Link v): v(v) {
  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmPrinter();
  llvm::InitializeNativeTargetAsmParser();
  
  std::string onError = "";
  auto eb = new llvm::EngineBuilder(std::unique_ptr<llvm::Module>(v->getModule()));
  engine = eb
    ->setErrorStr(&onError)
    .setEngineKind(llvm::EngineKind::JIT)
    .create();
  for (const auto& pair : nameToFunPtr) {
    auto funPtr = v->getModule()->getFunction(pair.first);
    if (funPtr == nullptr) continue; // Function not used
    engine->addGlobalMapping(funPtr, pair.second);
  }
  using namespace std::placeholders;
  if (auto funPtr = v->getModule()->getFunction("_xyl_dynAllocType")) {
    auto boundToThis = std::bind(&Runner::dynAllocType, this, _1);
    engine->addGlobalMapping(funPtr, &boundToThis);
  }
  engine->finalizeObject();
  if (onError != "") throw InternalError("ExecutionEngine error", {
    METADATA_PAIRS,
    {"supplied error string", onError}
  });
}

int Runner::run() {
  return engine->runFunctionAsMain(v->getEntryPoint(), {}, {});
}

void* Runner::dynAllocType(UniqueIdentifier typeId) {
  llvm::DataLayout d(v->getModule());
  auto id = *std::find_if(ALL(*v->getTypeSetPtr().get()), [=](auto tid) {
    return *tid == typeId;
  });
  return std::malloc(d.getTypeAllocSize(id->getAllocaType()));
}
