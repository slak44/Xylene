#ifndef LLVM_TEST_HPP
#define LLVM_TEST_HPP

#include <gtest/gtest.h>
#include <rapidxml_utils.hpp>

#include "utils/util.hpp"
#include "llvm/compiler.hpp"
#include "parser/xmlParser.hpp"

class LLVMCompilerTest: public ::testing::Test {
protected:
  XMLParser xpx = XMLParser();
  
  inline rapidxml::file<> xmlFile(std::string path) {
    return rapidxml::file<>(path.c_str());
  }
  
  inline void noThrowOnCompile(std::string xmlFilePath) {
    xpx.parse(xmlFile(xmlFilePath));
    CompileVisitor::Link visitor = CompileVisitor::create(globalContext, xmlFilePath, xpx.getTree());
    try {
      visitor->visit();
    } catch (InternalError& err) {
      EXPECT_NO_THROW(throw InternalError(err));
      println(err.what());
    }
    visitor->getModule()->dump();
  }
};

TEST_F(LLVMCompilerTest, ExitCodes) {
  noThrowOnCompile("tests/data/llvm/exit_code.xml");
  noThrowOnCompile("tests/data/llvm/stupid_return.xml");
  noThrowOnCompile("tests/data/llvm/stored_return.xml");
}

TEST_F(LLVMCompilerTest, Declarations) {
  noThrowOnCompile("tests/data/llvm/primitive.xml");
}

TEST_F(LLVMCompilerTest, Branches) {
  noThrowOnCompile("tests/data/llvm/if.xml");
  noThrowOnCompile("tests/data/llvm/if-else.xml");
  noThrowOnCompile("tests/data/llvm/else-if.xml");
}

#endif
