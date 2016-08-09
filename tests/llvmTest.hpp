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
    CompileVisitor::Link visitor = CompileVisitor::create(globalContext, "Test Module", xpx.getTree());
    EXPECT_NO_THROW(visitor->visit());
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

#endif
