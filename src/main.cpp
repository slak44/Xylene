#include <tclap/CmdLine.h>
#include <llvm/IR/Module.h>
#include <fstream>
#include <sstream>
#include <chrono>

#include "utils/util.hpp"
#include "utils/error.hpp"
#include "lexer.hpp"
#include "parser/tokenParser.hpp"
#include "parser/xmlParser.hpp"
#include "llvm/compiler.hpp"
#include "llvm/runner.hpp"

enum ExitCodes: int {
  NORMAL_EXIT = 0, ///< Everything is OK
  TCLAP_ERROR = 11, ///< TCLAP did something bad, should not happen
  CLI_ERROR = 1, ///< The user used a wrong combination of options
  USER_PROGRAM_ERROR = 2, ///< The thing we had to lex/parse/run has an issue
  INTERNAL_ERROR = 3 ///< Unrecoverable error in this program, almost always a bug
};

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-declarations"
#ifdef __clang__
#pragma clang diagnostic ignored "-Wmissing-prototypes"
#endif

std::unique_ptr<AST> parseXML(fs::path filePath, std::string cliEval) {
  if (!filePath.empty()) {
    // Read from file
    auto file = rapidxml::file<>(filePath.c_str());
    return std::make_unique<AST>(XMLParser().parse(file).getTree());
  } else {
    // Read from CLI
    char* mutableCode = new char[cliEval.size() + 1];
    std::move(ALL(cliEval), mutableCode);
    mutableCode[cliEval.size()] = '\0';
    return std::make_unique<AST>(XMLParser().parse(mutableCode).getTree());
  }
}

std::vector<Token> tokenize(fs::path filePath, std::string cliEval) {
  if (!filePath.empty()) {
    std::ifstream file(filePath);
    std::stringstream buffer;
    buffer << file.rdbuf();
    return Lexer::tokenize(buffer.str(), filePath)->getTokens();
  }
  return Lexer::tokenize(cliEval, "<cli-eval>")->getTokens();
}

/// Throw if the assertion is false
void assertCliIntegrity(TCLAP::CmdLine& cmd, bool assertion, std::string message) {
  if (!assertion) return;
  TCLAP::ArgException arg(message);
  cmd.getOutput()->failure(cmd, arg);
}

/// Pseudo-main used to allow early returns while measuring execution time
int notReallyMain(int argc, const char* argv[]) {
  try {
    TCLAP::CmdLine cmd("Xylene", ' ', "pre-release");
    
    TCLAP::SwitchArg asXML("", "xml", "Read file using the XML parser", cmd);
    
    TCLAP::SwitchArg printTokens("", "tokens", "Print token list (if applicable)", cmd);
    TCLAP::SwitchArg printAST("", "ast", "Print AST (if applicable)", cmd);
    TCLAP::SwitchArg printIR("", "ir", "Print LLVM IR (if applicable)", cmd);
    
    TCLAP::SwitchArg doNotParse("", "no-parse", "Don't parse the token list", cmd);
    TCLAP::SwitchArg doNotRun("", "no-run", "Don't execute the AST", cmd);
    
    std::vector<std::string> runnerValues {"interpret", "compile"};
    TCLAP::ValuesConstraint<std::string> runnerConstraint(runnerValues);
    TCLAP::ValueArg<std::string> runner("r", "runner", "How to run this code", false,
      "interpret", &runnerConstraint, cmd, nullptr);
    
    TCLAP::ValueArg<std::string> code("e", "eval", "Code to evaluate", false,
      std::string(), "string", cmd, nullptr);
    TCLAP::ValueArg<std::string> filePath("f", "file", "Load code from this file",
      false, std::string(), "path", cmd, nullptr);
    TCLAP::ValueArg<std::string> outPath("o", "output", "Write exe to this file",
      false, std::string(), "path", cmd, nullptr);
    cmd.parse(argc, argv);
    
    // There must be at least one input
    assertCliIntegrity(cmd, code.getValue().empty() && filePath.getValue().empty(),
      "Must specify either option -e or -f");
    
    // There is not much you can do to the XML without parsing it
    assertCliIntegrity(cmd, asXML.getValue() && doNotParse.getValue(),
      "--no-parse and --xml are incompatible");
    
    // The XML parser does not use tokens
    assertCliIntegrity(cmd, asXML.getValue() && printTokens.getValue(),
      "--tokens and --xml are incompatible");
    
    // Can't print AST without creating it first
    assertCliIntegrity(cmd, printAST.getValue() && doNotParse.getValue(),
      "--no-parse and --ast are incompatible");
      
    // Can't print IR without parsing the AST
    assertCliIntegrity(cmd, printIR.getValue() && doNotParse.getValue(),
      "--no-parse and --ir are incompatible");
      
    std::unique_ptr<AST> ast;
    
    if (asXML.getValue()) {
      ast = parseXML(filePath.getValue(), code.getValue());
    } else {
      auto tokens = tokenize(filePath.getValue(), code.getValue());
      if (printTokens.getValue()) for (auto tok : tokens) println(tok);
      
      if (doNotParse.getValue()) return NORMAL_EXIT;
      ast = std::make_unique<AST>(TokenParser().parse(tokens).getTree());
    }
    
    if (printAST.getValue()) ast->print();
    if (doNotRun.getValue()) return NORMAL_EXIT;
    
    ModuleCompiler::Link mc = ModuleCompiler::create("Command Line Module", *ast);
    mc->visit();
    if (printIR.getValue()) mc->getModule()->dump();
        
    if (runner.getValue() == "interpret") {
      return Runner(mc).run();
    } else if (runner.getValue() == "compile") {
      Compiler(std::unique_ptr<llvm::Module>(mc->getModule()),
        filePath.getValue(), outPath.getValue()).compile();
      return NORMAL_EXIT;
    }
  } catch (const TCLAP::ExitException&) {
    return CLI_ERROR;
  } catch (const TCLAP::ArgException& arg) {
    println("TCLAP error", arg.error(), "for", arg.argId());
    return TCLAP_ERROR;
  } catch (const Error& err) {
    println(err.what());
    return USER_PROGRAM_ERROR;
  }
  // If we're debugging, crash the program on InternalError
  #ifndef CRASH_ON_INTERNAL_ERROR
  catch (const InternalError& err) {
    println(err.what());
    return INTERNAL_ERROR;
  }
  #endif
  return 0;
}

#pragma GCC diagnostic pop

/// Measures execution time if XYLENE_MEASURE_TIME is defined \see notReallyMain
int main(int argc, const char* argv[]) {
  #ifdef XYLENE_MEASURE_TIME
  auto begin = std::chrono::steady_clock::now();
  #endif
  
  int exitCode = notReallyMain(argc, argv);
  
  #ifdef XYLENE_MEASURE_TIME
  auto end = std::chrono::steady_clock::now();
  println(
    "Time diff:",
    std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count(),
    "μs"
  );
  #endif

  return exitCode;
}
