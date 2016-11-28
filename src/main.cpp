#include <tclap/CmdLine.h>
#include <llvm/IR/Module.h>
#include <fstream>
#include <sstream>

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
  CLI_ERROR = 1, ///< The user is retar-- uh I mean the user used a wrong option
  USER_PROGRAM_ERROR = 2, ///< The thing we had to lex/parse/run has an issue
  INTERNAL_ERROR = 3 ///< Unrecoverable error in this program, almost always a bug
};

int main(int argc, const char* argv[]) {
  try {
    TCLAP::CmdLine cmd("Xylene", ' ', "pre-release");
    
    TCLAP::SwitchArg asXML("", "xml", "Read file using the XML parser", cmd);
    
    TCLAP::SwitchArg printTokens("", "tokens", "Print token list", cmd);
    TCLAP::SwitchArg printAST("", "ast", "Print AST (if applicable)", cmd);
    TCLAP::SwitchArg printIR("", "ir", "Print LLVM IR (if applicable)", cmd);
    
    TCLAP::SwitchArg doNotParse("", "no-parse", "Don't parse the token list", cmd);
    TCLAP::SwitchArg doNotRun("", "no-run", "Don't execute the AST", cmd);
    
    std::vector<std::string> runnerValues {"llvm-lli", "llvm-llc"};
    TCLAP::ValuesConstraint<std::string> runnerConstraint(runnerValues);
    TCLAP::ValueArg<std::string> runner("r", "runner", "How to run this code", false,
      "llvm-lli", &runnerConstraint, cmd, nullptr);
    
    TCLAP::ValueArg<std::string> code("e", "eval", "Code to evaluate", false,
      std::string(), "string", cmd, nullptr);
    TCLAP::ValueArg<std::string> filePath("f", "file", "Load code from this file",
      false, std::string(), "path", cmd, nullptr);
    cmd.parse(argc, argv);
    
    if (code.getValue().empty() && filePath.getValue().empty()) {
      TCLAP::ArgException arg("Must specify either option -e or -f");
      cmd.getOutput()->failure(cmd, arg);
    }
    
    std::unique_ptr<AST> ast;
    
    if (asXML.getValue()) {
      if (doNotParse.getValue()) return NORMAL_EXIT;
      if (filePath.getValue().empty()) {
        TCLAP::ArgException arg("XML option only works with files");
        cmd.getOutput()->failure(cmd, arg);
      }
      ast = std::make_unique<AST>(XMLParser().parse(rapidxml::file<>(filePath.getValue().c_str())).getTree());
    } else {
      std::string input;
      if (!filePath.getValue().empty()) {
        std::ifstream file(filePath.getValue());
        std::stringstream buffer;
        buffer << file.rdbuf();
        input = buffer.str();
      } else {
        input = code.getValue();
      }
      
      auto lx = Lexer();
      lx.tokenize(input, filePath.getValue().empty() ? "<cli-eval>" : filePath.getValue());
      if (printTokens.getValue()) for (auto tok : lx.getTokens()) println(tok);
      
      if (doNotParse.getValue()) return NORMAL_EXIT;
      ast = std::make_unique<AST>(TokenParser().parse(lx.getTokens()).getTree());
    }
    
    if (printAST.getValue()) ast->print();
    if (doNotRun.getValue()) return NORMAL_EXIT;
    
    CompileVisitor::Link v = CompileVisitor::create("Command Line Module", *ast);
    v->visit();
    if (printIR.getValue()) v->getModule()->dump();
        
    if (runner.getValue() == "llvm-lli") {
      return Runner(v).run();
    } else if (runner.getValue() == "llvm-llc") {
      println("Not yet implemented!");
      // TODO
    }
  } catch (const TCLAP::ExitException& arg) {
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
