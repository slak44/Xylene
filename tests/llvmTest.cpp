#include <gtest/gtest.h>
#include <rapidxml_utils.hpp>

#include "llvm/compiler.hpp"
#include "parser/xmlParser.hpp"

class LLVMCompilerTest: public ::testing::Test {
protected:
  XMLParser xpx = XMLParser();
  
  inline rapidxml::file<> xmlFile(std::string path) {
    path = DATA_PARENT_DIR + ("/" + path);
    return rapidxml::file<>(path.c_str());
  }
  
  inline void compile(std::string xmlFilePath) {
    xpx.parse(xmlFile(xmlFilePath));
    CompileVisitor::Link visitor = CompileVisitor::create(globalContext, xmlFilePath, xpx.getTree());
    visitor->visit();
  }
  
  inline void noThrowOnCompile(std::string xmlFilePath) {
    xpx.parse(xmlFile(xmlFilePath));
    CompileVisitor::Link visitor = CompileVisitor::create(globalContext, xmlFilePath, xpx.getTree());
    try {
      visitor->visit();
    } catch (InternalError& err) {
      EXPECT_NO_THROW(throw InternalError(err));
      println(err.what());
      visitor->getModule()->dump();
    }
  }
};

TEST_F(LLVMCompilerTest, ExitCodes) {
  noThrowOnCompile("data/llvm/exit_code.xml");
  noThrowOnCompile("data/llvm/stored_return.xml");
}

TEST_F(LLVMCompilerTest, Declarations) {
  noThrowOnCompile("data/llvm/primitive.xml");
}

TEST_F(LLVMCompilerTest, Assignment) {
  noThrowOnCompile("data/llvm/return_assign.xml");
  EXPECT_THROW(compile("data/llvm/assign_to_literal.xml"), Error);
}

TEST_F(LLVMCompilerTest, Branches) {
  noThrowOnCompile("data/llvm/if.xml");
  noThrowOnCompile("data/llvm/if-else.xml");
  noThrowOnCompile("data/llvm/else-if.xml");
}
