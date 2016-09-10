#ifndef RUNNER_HPP
#define RUNNER_HPP

#include <llvm/IR/Module.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/Interpreter.h>
#include <llvm/Support/TargetSelect.h>

#include "llvm/compiler.hpp"

/// exit code + stdout
using ProgramResult = std::pair<int, std::string>;

class Runner {
private:
  llvm::ExecutionEngine* engine;
  CompileVisitor::Link v;
public:
  Runner(CompileVisitor::Link v);
  int run();
  ProgramResult runAndCapture();
};

#endif
