#include <sstream>
#include <gtest/gtest.h>
#include <rapidxml_utils.hpp>
#include <llvm/IR/Module.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/Interpreter.h>
#include <llvm/Support/TargetSelect.h>

#include "llvm/compiler.hpp"
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
  
  // exit code + stdout
  using R = std::pair<int, std::string>;
  
  inline R compileAndRun(std::string xmlFilePath) {
    auto v = compile(xmlFilePath);

    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    llvm::InitializeNativeTargetAsmParser();
    
    std::string onError = "";
    auto eb = new llvm::EngineBuilder(std::unique_ptr<llvm::Module>(v->getModule()));
    llvm::ExecutionEngine* ee = eb
      ->setEngineKind(llvm::EngineKind::Interpreter)
      .setErrorStr(&onError)
      .create();
    auto main = v->getEntryPoint();
    if (onError != "") throw InternalError("ExecutionEngine error", {
      METADATA_PAIRS,
      {"supplied error string", onError}
    });
    // Capture stdout
    char buf[8192];
    std::fflush(stdout);
    auto stdoutOrig = dup(STDOUT_FILENO);
    std::freopen("/dev/null", "a", stdout);
    std::setbuf(stdout, buf);
    // Run program
    int exitCode = ee->runFunctionAsMain(main, {}, {});
    // Stop capturing stdout
    std::fflush(stdout);
    dup2(stdoutOrig, STDOUT_FILENO);
    return {exitCode, std::string(buf)};
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
  EXPECT_EQ(compileAndRun("data/llvm/functions/foreign.xml"), R({0, "A"}));
}
