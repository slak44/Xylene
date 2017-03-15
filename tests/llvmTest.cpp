#include <sstream>
#include <gtest/gtest.h>
#include <rapidxml_utils.hpp>

#include "test.hpp"
#include "llvm/compiler.hpp"
#include "llvm/runner.hpp"
#include "parser/xmlParser.hpp"

class LLVMCompilerTest: public ::testing::Test, public ExternalProcessCompiler {
protected:
  inline rapidxml::file<> xmlFile(fs::path relativePath) {
    return rapidxml::file<>((DATA_PARENT_DIR / relativePath).c_str());
  }
  
  inline ModuleCompiler::Link compile(fs::path xmlFilePath) {
    auto mc = ModuleCompiler::create(
      {}, xmlFilePath, XMLParser::parse(xmlFile(xmlFilePath)), true);
    mc->compile();
    if (printIr) mc->getModule()->dump();
    return mc;
  }
  
  inline void noThrowOnCompile(fs::path xmlFilePath) {
    auto mc = ModuleCompiler::create(
      {}, xmlFilePath, XMLParser::parse(xmlFile(xmlFilePath)), true);
    try {
      mc->compile();
      if (printIr) mc->getModule()->dump();
    } catch (InternalError& err) {
      EXPECT_NO_THROW(throw err);
      println(err.what());
      mc->getModule()->dump();
    }
  }
};

TEST_F(LLVMCompilerTest, Loops) {
  noThrowOnCompile("data/llvm/loops/complete.xml");
  noThrowOnCompile("data/llvm/loops/no_init.xml");
  noThrowOnCompile("data/llvm/loops/no_condition.xml");
  noThrowOnCompile("data/llvm/loops/no_update.xml");
}

TEST_F(LLVMCompilerTest, ExitCodes) {
  noThrowOnCompile("data/llvm/exit_code.xml");
  noThrowOnCompile("data/llvm/stored_return.xml");
}

TEST_F(LLVMCompilerTest, Branches) {
  noThrowOnCompile("data/llvm/if.xml");
  noThrowOnCompile("data/llvm/if_else.xml");
  noThrowOnCompile("data/llvm/else_if.xml");
}

TEST_F(LLVMCompilerTest, Declarations) {
  noThrowOnCompile("data/llvm/declarations/primitive.xml");
  noThrowOnCompile("data/llvm/declarations/multiple_declare.xml");
  EXPECT_THROW(compile("data/llvm/declarations/simple_type_mismatch.xml"), Error);
  EXPECT_THROW(compile("data/llvm/declarations/multiple_type_mismatch.xml"), Error);
}

TEST_F(LLVMCompilerTest, Assignment) {
  noThrowOnCompile("data/llvm/return_assign.xml");
  EXPECT_THROW(compile("data/llvm/assign_to_literal.xml"), Error);
}

TEST_F(LLVMCompilerTest, Functions) {
  noThrowOnCompile("data/llvm/functions/function.xml");
  noThrowOnCompile("data/llvm/functions/no_args.xml");
  noThrowOnCompile("data/llvm/functions/void_ret.xml");
  if (spawnProcs) EXPECT_EQ(
    compileAndRun("data/llvm/functions/foreign.xml", "--xml"),
    ProgramResult({0, "A", ""})
  );
}

TEST_F(LLVMCompilerTest, UserTypes) {
  noThrowOnCompile("data/llvm/types/simple_type.xml");
  noThrowOnCompile("data/llvm/types/static_method.xml");
}
