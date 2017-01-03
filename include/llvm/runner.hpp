#ifndef RUNNER_HPP
#define RUNNER_HPP

#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/Interpreter.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/Object/ObjectFile.h>
#include <tuple>

#include "runtime/standard.hpp"
#include "llvm/compiler.hpp"

/// exit code + stdout + stderr
using ProgramResult = std::tuple<int, std::string, std::string>;

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
  /**
    \brief Runs the program and captures stdout (unreliable)
    \deprecated spawn another process instead, this doesn't always work
  */
  ProgramResult runAndCapture();
};

#endif
