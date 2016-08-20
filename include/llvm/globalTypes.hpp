#ifndef GLOBAL_TYPES_HPP
#define GLOBAL_TYPES_HPP

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>

static const uint BITS_PER_INT = 64;

/// A reference to the global LLVM context
extern llvm::LLVMContext& globalContext;

/// Integral type
extern llvm::IntegerType* integerType;
/// Floating-point type
extern llvm::Type* floatType;
/// Boolean type
extern llvm::IntegerType* booleanType;

/// Used for throwing consistent type mismatch errors
const std::string typeMismatchErrorString = "No operation available for given operands";

#endif
