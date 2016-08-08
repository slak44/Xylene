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
};

TEST_F(LLVMCompilerTest, TrivialExpression) {
  xpx.parse(xmlFile("tests/data/trivial_expr.xml"));
  CompileVisitor::Link visitor = CompileVisitor::create(globalContext, "Test Module", xpx.getTree());
  visitor->visit();
  visitor->getModule()->dump();
  EXPECT_EQ(1, 1);
}

#endif
