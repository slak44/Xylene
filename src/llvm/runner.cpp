#include "llvm/runner.hpp"

Runner::Runner(CompileVisitor::Link v): v(v) {
  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmPrinter();
  llvm::InitializeNativeTargetAsmParser();
  
  std::string onError = "";
  auto eb = new llvm::EngineBuilder(std::unique_ptr<llvm::Module>(v->getModule()));
  engine = eb
    ->setEngineKind(llvm::EngineKind::Interpreter)
    .setErrorStr(&onError)
    .create();
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
