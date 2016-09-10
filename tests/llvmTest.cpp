#include <sstream>
#include <gtest/gtest.h>
#include <rapidxml_utils.hpp>

#include "llvm/compiler.hpp"
#include "llvm/runner.hpp"
#include "parser/xmlParser.hpp"

class LLVMCompilerTest: public ::testing::Test {
protected:
  XMLParser xpx = XMLParser();
  
  inline rapidxml::file<> xmlFile(std::string path) {
    path = DATA_PARENT_DIR + ("/" + path);
    return rapidxml::file<>(path.c_str());
  }
  
  inline CompileVisitor::Link compile(std::string xmlFilePath) {
    xpx.parse(xmlFile(xmlFilePath));
    CompileVisitor::Link visitor = CompileVisitor::create(xmlFilePath, xpx.getTree());
    visitor->visit();
    return visitor;
  }
  
  inline void noThrowOnCompile(std::string xmlFilePath) {
    xpx.parse(xmlFile(xmlFilePath));
    CompileVisitor::Link visitor = CompileVisitor::create(xmlFilePath, xpx.getTree());
    try {
      visitor->visit();
    } catch (InternalError& err) {
      EXPECT_NO_THROW(throw InternalError(err));
      println(err.what());
      visitor->getModule()->dump();
    }
  }
  
  inline ProgramResult compileAndRun(std::string xmlFilePath) {
    auto v = compile(xmlFilePath);
    Runner r(v);
    return r.runAndCapture();
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
  noThrowOnCompile("data/llvm/primitive.xml");
}

TEST_F(LLVMCompilerTest, Assignment) {
  noThrowOnCompile("data/llvm/return_assign.xml");
  EXPECT_THROW(compile("data/llvm/assign_to_literal.xml"), Error);
}

TEST_F(LLVMCompilerTest, Functions) {
  noThrowOnCompile("data/llvm/functions/function.xml");
  noThrowOnCompile("data/llvm/functions/no_args.xml");
  noThrowOnCompile("data/llvm/functions/void_ret.xml");
  EXPECT_EQ(compileAndRun("data/llvm/functions/foreign.xml"), ProgramResult({0, "A"}));
}
