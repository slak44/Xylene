#include "llvm/runner.hpp"

Runner::Runner(CompileVisitor::Link v): v(v) {
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
  engine->finalizeObject();
  if (onError != "") throw InternalError("ExecutionEngine error", {
    METADATA_PAIRS,
    {"supplied error string", onError}
  });
}

int Runner::run() {
  return engine->runFunctionAsMain(v->getEntryPoint(), {}, {});
}

ProgramResult Runner::runAndCapture() {
  // Capture stdout
  char buf[8192];
  std::fflush(stdout);
  std::freopen("/dev/null", "a", stdout);
  std::setbuf(stdout, buf);
  // Run program
  int exitCode = run();
  // Stop capturing stdout
  std::freopen("/dev/tty", "a", stdout);
  return {exitCode, buf};
}
