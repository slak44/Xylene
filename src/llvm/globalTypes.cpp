#include "llvm/globalTypes.hpp"

llvm::LLVMContext& globalContext(llvm::getGlobalContext());

llvm::IntegerType* integerType = llvm::IntegerType::get(globalContext, BITS_PER_INT);
llvm::Type* floatType = llvm::Type::getDoubleTy(globalContext);
llvm::IntegerType* booleanType = llvm::Type::getInt1Ty(globalContext);
