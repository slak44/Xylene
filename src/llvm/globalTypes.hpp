#ifndef GLOBAL_TYPES_HPP
#define GLOBAL_TYPES_HPP

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>

llvm::LLVMContext& globalContext(llvm::getGlobalContext());

static const uint BITS_PER_INT = 64;
llvm::IntegerType* integerType = llvm::IntegerType::get(globalContext, BITS_PER_INT);
llvm::Type* floatType = llvm::Type::getDoubleTy(globalContext);
llvm::IntegerType* booleanType = llvm::Type::getInt1Ty(globalContext);

#endif
