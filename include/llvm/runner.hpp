#ifndef RUNNER_HPP
#define RUNNER_HPP

#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/Object/ObjectFile.h>
#include <tuple>

#include "runtime/runtime.hpp"
#include "llvm/compiler.hpp"

/// Maps runtime function names to pointers to those functions.
const std::unordered_map<std::string, void*> nameToFunPtr {
  {"printC", reinterpret_cast<void*>(printC)},
  {"_xyl_typeErrIfIncompatible", reinterpret_cast<void*>(_xyl_typeErrIfIncompatible)},
  {"_xyl_typeErrIfIncompatibleTid", reinterpret_cast<void*>(_xyl_typeErrIfIncompatibleTid)},
};

/// exit code + stdout + stderr
using ProgramResult = std::tuple<int, std::string, std::string>;

/**
  \brief Takes a ModuleCompiler and executes the module it parsed.
*/
class Runner {
private:
  llvm::ExecutionEngine* engine;
  ModuleCompiler::Link v;
public:
  Runner(ModuleCompiler::Link v);
  
  /// \return exit code of executed program
  int run();
};

#endif
