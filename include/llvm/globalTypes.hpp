#ifndef GLOBAL_TYPES_HPP
#define GLOBAL_TYPES_HPP

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>

static const uint BITS_PER_INT = 64;

extern llvm::LLVMContext& globalContext;

extern llvm::IntegerType* integerType;
extern llvm::Type* floatType;
extern llvm::IntegerType* booleanType;

#endif
