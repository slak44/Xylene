#ifndef RUNNER_HPP
#define RUNNER_HPP

#include <llvm/IR/Module.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/Interpreter.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/Object/ObjectFile.h>
#include <llvm/Support/TargetSelect.h>

#include "standard.hpp"
#include "llvm/compiler.hpp"

/// exit code + stdout
using ProgramResult = std::pair<int, std::string>;

/**
  \brief Takes a CompileVisitor and executes the module it parsed.
*/
class Runner {
private:
  llvm::ExecutionEngine* engine;
  CompileVisitor::Link v;
public:
  Runner(CompileVisitor::Link v);
  /// \return exit code of executed program
  int run();
  /// \see ProgramResult
  ProgramResult runAndCapture();
};

#endif
