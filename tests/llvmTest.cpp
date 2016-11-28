#include <sstream>
#include <gtest/gtest.h>
#include <rapidxml_utils.hpp>
#include <experimental/filesystem>
#include <tiny-process-library/process.hpp>

#include "llvm/compiler.hpp"
#include "llvm/runner.hpp"
#include "parser/xmlParser.hpp"

extern "C" bool printIr;

namespace fs = std::experimental::filesystem;

class LLVMCompilerTest: public ::testing::Test {
protected:
  XMLParser xpx = XMLParser();
  
  inline std::string absoluteXmlPath(fs::path relativePath) {
    fs::path dataParentDir = DATA_PARENT_DIR/relativePath;
    return dataParentDir;
  }
  
  inline rapidxml::file<> xmlFile(fs::path path) {
    return rapidxml::file<>(absoluteXmlPath(path).c_str());
  }
  
  inline CompileVisitor::Link compile(fs::path xmlFilePath) {
    xpx.parse(xmlFile(xmlFilePath));
    CompileVisitor::Link visitor = CompileVisitor::create(xmlFilePath, xpx.getTree());
    visitor->visit();
    if (printIr) visitor->getModule()->dump();
    return visitor;
  }
  
  inline void noThrowOnCompile(fs::path xmlFilePath) {
    xpx.parse(xmlFile(xmlFilePath));
    CompileVisitor::Link visitor = CompileVisitor::create(xmlFilePath, xpx.getTree());
    try {
      visitor->visit();
      if (printIr) visitor->getModule()->dump();
    } catch (InternalError& err) {
      EXPECT_NO_THROW(throw err);
      println(err.what());
      visitor->getModule()->dump();
    }
  }
  
  inline ProgramResult compileAndRun(fs::path xmlFilePath) {
    std::string stdout, stderr;
    auto printIrSwitch = printIr ? " --ir" : "";
    Process self(std::string(FULL_PROGRAM_PATH) + " --xml -f " + absoluteXmlPath(xmlFilePath) + printIrSwitch, "",
    [&stdout](const char* bytes, std::size_t) {
      stdout = bytes;
    },
    [&stderr](const char* bytes, std::size_t) {
      stderr = bytes;
    });
    auto exitCode = self.get_exit_status();
    return {exitCode, stdout, stderr};
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
  EXPECT_EQ(compileAndRun("data/llvm/functions/foreign.xml"), ProgramResult({0, "A", ""}));
}

TEST_F(LLVMCompilerTest, UserTypes) {
  noThrowOnCompile("data/llvm/types/simple_type.xml");
  noThrowOnCompile("data/llvm/types/static_method.xml");
}
