#include <iostream>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <algorithm>
#include <functional>
#include <unordered_map>
#include <boost/program_options.hpp>

#include "global.hpp"
#include "operators.hpp"
#include "objects.hpp"
#include "builtins.hpp"
#include "tokens.hpp"
#include "nodes.hpp"
#include "functions.hpp"
#include "ast.hpp"
#include "operator_maps.hpp"

namespace lang {
  Type* Boolean::booleanType = new Type("Boolean", {}, {});
  Type* String::stringType = new Type("String", {}, {});
  Type* Float::floatType = new Type("Float", {}, {});
  Type* Integer::integerType = new Type("Integer", {}, {});
  class Parser {
  private:
    std::vector<Token> tokens {};
    inline void skipCharacters(uint64& i, int by) {i += by;}
    // If we're already at the next character, but the loop will increment i by 1
    inline void preventIncrement(uint64& i) {i--;}
    
    std::vector<Token> variables {};
    std::vector<std::string> types {"Integer", "Float", "String", "Boolean"};
    const std::vector<std::string> keywords {
      "define",
      "function", "return",
      "type", "constructor", "public", "protected", "private",
      "if", "while"
    };
    // Technically keywords, but constructs for the purpose of this parser
    const std::vector<std::string> constructKeywords {
      "do", "end",
      "else",
    };
    const std::unordered_map<char, char> singleCharEscapeSeqences {
      {'a', '\a'},
      {'b', '\b'},
      {'f', '\f'},
      {'n', '\n'},
      {'r', '\r'},
      {'t', '\t'},
      {'v', '\v'},
      {'\\', '\\'},
      {'\'', '\''},
      {'"', '\"'},
      {'?', '\?'},
    };
  public:
    AST tree = AST();
    
    Parser(std::string code) {
      if (PARSER_PRINT_INPUT) print(code, "\n");
      try {
        tokenize(code);
        if (PARSER_PRINT_TOKENS) for (auto tok : tokens) print(tok, "\n");
        if (PARSER_PRINT_AS_EXPR) {
          for (auto tok : ExpressionNode(tokens).getRPNOutput()) print(tok.data, " ");
          print("\n");
        }
        buildTree(tokens);
      } catch (Error &e) {
        print(e.toString());
      }
    }
    
    void tokenize(std::string code) {
      uint lines = 1;
      bool isInComment = false;
      for (uint64 i = 0; i < code.length(); ++i) {
        // Line comments
        if (code[i] == '/' && code[i + 1] == '/') while (code[i] != '\n' && code[i] != '\0') skipCharacters(i, 1);
        
        // Count lines
        if (code[i] == '\n') lines++;
        
        // Multi-line comments
        if (code[i] == '/' && code[i + 1] == '*') {
          i += 2;
          isInComment = true;
        }
        if (code[i] == '*' && code[i + 1] == '/') {
          i += 2;
          isInComment = false;
        }
        
        // Ignore comments
        if (isInComment) continue;
        
        // Ignore whitespace
        if (isspace(code[i])) continue;
        
        // Various constructs
        if (code[i] == '{' || code[i] == '}' ||
            code[i] == '[' || code[i] == ']' ||
            code[i] == '(' || code[i] == ')' ||
            code[i] == ':' || code[i] == ';') {
          tokens.push_back(Token(std::string(1, code[i]), CONSTRUCT, lines));
          continue;
        }
        
        // Fat arrow for function return types
        if (code[i] == '=' && code[i + 1] == '>') {
          tokens.push_back(Token("=>", CONSTRUCT, lines));
          skipCharacters(i, 2);
          continue;
        }
        
        // Operators
        auto initTokSize = tokens.size();
        std::for_each(opList.begin(), opList.end(), [this, initTokSize, code, i, lines](Operator& op) {
          if (initTokSize < tokens.size()) return; // If there are more tokens than before for_each, the operator was already added, so return.
          if (op.toString().length() + i > code.length()) return; // If the operator is longer than the source string, ignore it.
          if (op.toString() == code.substr(i, op.toString().length())) {
            Operator* tmp = &op;
            if (tmp->toString() == "++" || tmp->toString() == "--") {
              // Prefix version
              if (tokens.back().type == OPERATOR || tokens.back().type == CONSTRUCT) tmp = new Operator(tmp->toString(), 12, ASSOCIATE_FROM_RIGHT, UNARY);
              // Postfix version
              else tmp = new Operator(tmp->toString(), 13, ASSOCIATE_FROM_LEFT, UNARY);
            }
            if (tmp->toString() == "+" || tmp->toString() == "-") {
              // Unary version
              if (tokens.back().type == OPERATOR || tokens.back().type == CONSTRUCT) tmp = new Operator(tmp->toString(), 12, ASSOCIATE_FROM_RIGHT, UNARY);
              // Binary version
              else tmp = new Operator(tmp->toString(), 10);
            }
            if (PARSER_PRINT_OPERATOR_TOKENS) print("Parser found Token with Operator ", *tmp, ", at address ", tmp, "\n");
            tokens.push_back(Token(tmp, OPERATOR, lines));
          }
        });
        
        if (initTokSize < tokens.size()) {
          skipCharacters(i, tokens.back().data.length() - 1);
          continue; // Token added, continue.
        }
        
        // Numbers
        if (isdigit(code[i])) {
          int base = 10;
          std::string current = "";
          if (code[i] == '0') {
            switch (code[i + 1]) {
              case 'x': base = 16; skipCharacters(i, 2); break;
              case 'b': base =  2; skipCharacters(i, 2); break;
              case 'o': base =  8; skipCharacters(i, 2); break;
              default: break;
            }
          }
          bool isFloat = false;
          while (isdigit(code[i]) || code[i] == '.') {
            current += code[i];
            if (code[i] == '.') {
              if (isFloat) throw Error("Malformed float, multiple decimal points: \"" + current + "\"", "SyntaxError", lines);
              isFloat = true;
            }
            skipCharacters(i, 1);
          }
          preventIncrement(i);
          if (current[current.length() - 1] == '.') throw Error("Malformed float, missing digits after decimal point: \"" + current + "\"", "SyntaxError", lines);
          if (base != 10 && isFloat) throw Error("Floating point numbers must be used with base 10 numbers: \"" + current + "\"", "SyntaxError", lines);
          Token t;
          if (isFloat) t = Token(new Float(current), FLOAT, lines);
          else t = Token(new Integer(current, base), INTEGER, lines);
          tokens.push_back(t);
          continue;
        }
        
        // String literals
        if (code[i] == '"') {
          skipCharacters(i, 1); // Skip the double quote
          std::string current = "";
          while (code[i] != '"') {
            if (code[i] == '\\') {
              code.erase(i, 1); // Erase the slash
              try {
                code[i] = singleCharEscapeSeqences.at(code[i]); // Now that the slash is gone, code[i] is the escaped char, eg the `n` in \n
              } catch (std::out_of_range& oor) {
                // This error means it wasn't found in the escape sequence map
                if (code[i] == 'x') {
                  std::string hexNumber = "" + code[i + 1] + code[i + 2];
                  code.erase(i + 1, 2); // Erase the 2 digits after the `x`
                  code[i] = std::stoi(hexNumber, 0, 16); // Replace `x` with escaped char
                } else if (isdigit(code[i])) {
                  std::string octalNumber = "" + code[i] + code[i + 1] + code[i + 2];
                  code.erase(i + 1, 2); // Erase the 2 last digits
                  code[i] = std::stoi(octalNumber, 0, 8); // Replace the remaining digit with escaped char
                } else throw Error("Invalid escape code", "SyntaxError", lines);
              }
            }
            current += code[i];
            skipCharacters(i, 1);
          }
          // Don't call preventIncrement here so the other double quote is skipped
          tokens.push_back(Token(new String(current), STRING, lines));
          continue;
        }
        
        // Others
        Token token = Token();
        while (!isspace(code[i])) {
          if (isReservedChar(code[i]) || code[i] == '\0') break;
          token.data += code[i];
          skipCharacters(i, 1);
        }
        // TODO: rewrite those using else if
        // Check if the thing is a property name
        if (tokens.size() > 0 && tokens.back().type == OPERATOR && tokens.back().data == ".") {
          token.type = MEMBER;
          token.typeData = new Name(token.data);
        }
        // Check if the thing references a type
        if (contains(token.data, types)) {
          token.type = TYPE;
        }
        // Check if the thing is a Boolean
        if (token.data == "true" || token.data == "false") {
          token.type = BOOLEAN;
          token.typeData = new Boolean(token.data);
        }
        // Check if the thing references a variable
        for (auto tok : variables)
          if (tok.data == token.data) {
            token = tok;
            break;
          }
        // Check if the thing is a new variable
        if (tokens.size() > 0 && ((tokens.back().type == KEYWORD && tokens.back().data == "define") || tokens.back().type == TYPE)) {
          token.type = VARIABLE;
          variables.push_back(token);
        }
        // Check if the thing is a keyword
        if (contains(token.data, keywords)) token.type = KEYWORD;
        if (contains(token.data, constructKeywords)) token.type = CONSTRUCT;
        
        // Push token
        preventIncrement(i);
        token.line = lines;
        tokens.push_back(token);
      }
    }
    
    ExpressionNode* evaluateCondition(std::vector<Token>& toks) {
      std::vector<Token> exprToks = std::vector<Token>(toks.begin() + 1, toks.end() - 1);
      ExpressionNode* condition = new ExpressionNode(exprToks);
      condition->buildSubtree();
      return condition;
    }
    
    DeclarationNode* buildDeclaration(std::vector<Token> withoutTypes, TypeList types) {
      if (withoutTypes[0].type != VARIABLE) throw Error("Invalid identifier", "SyntaxError", withoutTypes[0].line);
      DeclarationNode* decl = new DeclarationNode(types, withoutTypes[0]);
      decl->setLineNumber(withoutTypes[0].line);
      if (withoutTypes[1].data != ";" || withoutTypes[1].type != CONSTRUCT) {
        ExpressionNode* expr = new ExpressionNode(withoutTypes);
        expr->buildSubtree();
        decl->addChild(expr);
      }
      return decl;
    }
    
    ExpressionNode* buildExpression(std::vector<Token> toks) {
      ExpressionNode* expr = new ExpressionNode(toks);
      expr->buildSubtree();
      expr->setLineNumber(toks[0].line);
      return expr;
    }
    
    void resolveTypeStatements(std::vector<Token>& toks, const std::function<void(ASTNode*)>& addToBlock) {
      // Check if it's a type list
      if (toks[0].type == TYPE && toks[1].type == OPERATOR && toks[1].data == ",") {
        TypeList typeList {};
        std::size_t count = 0;
        for (;; count++) {
          if (toks[count].type == TYPE) typeList.push_back(toks[count].data);
          else if (toks[count].type == OPERATOR && toks[count].data == ",") continue;
          else break;
        }
        auto decl = buildDeclaration(std::vector<Token>(toks.begin() + count, toks.end()), typeList);
        addToBlock(decl);
        return;
      }
      // Check if it's just one type in the declaration
      if (toks[0].type == TYPE && toks[1].type == VARIABLE) {
        auto decl = buildDeclaration(std::vector<Token>(toks.begin() + 1, toks.end()), {toks[0].data});
        addToBlock(decl);
        return;
      }
      
      // If it's none of the above, it's part of an expression
      auto expr = buildExpression(toks);
      addToBlock(expr);
    }
    
    TypeList tokenToStringTypeList(std::vector<Token> typeList) {
      TypeList stringList {};
      // Remove commas from type list
      std::remove_if(typeList.begin(), typeList.end(), [](Token& tok) {return tok.type == OPERATOR && tok.data == ",";});
      // Map each token to a string, creating the type list
      for (auto e : typeList) stringList.push_back(e.data);
      return stringList;
    }
    
    void resolveDefineStatements(std::vector<Token>& toks, const std::function<void(ASTNode*)>& addToBlock, std::vector<ASTNode*>& blockStack) {
      // Check if it's a simple variable declaration
      if (toks[1].type == VARIABLE) {
        auto decl = buildDeclaration(std::vector<Token>(toks.begin() + 1, toks.end()), {toks[0].data});
        addToBlock(decl);
        return;
      }
      // Check if it's a type declaration
      if (toks[1].data == "type" && toks[1].type == KEYWORD) {
        // TODO: handle type declaration
        return;
      }
      // Check if it's a function declaration
      if (toks[1].data == "function" && toks[1].type == KEYWORD) {
        Arguments* args = new Arguments();
        TypeList returnTypes;
        // Parse arguments
        std::size_t pos = 3;
        while (toks[pos].data != "=>" && toks[pos].data != "do") {
          if (toks[pos].data == "[") {
            std::vector<Token> argumentData;
            pos++; // Ignore the leading bracket '['
            while (toks[pos].data != "]") {
              argumentData.push_back(toks[pos]);
              pos++;
              if (pos >= toks.size()) throw Error("Missing ] in declaration of function " + toks[2].data, "SyntaxError", toks[2].line);
            }
            pos++; // Ignore the trailing bracket ']'
            TypeList typeList;
            std::string name;
            if (argumentData.size() > 2 && argumentData[2].data == ":") {
              // Is the name prefixed
              name = argumentData[0].data;
              argumentData = std::vector<Token>(argumentData.begin() + 2, argumentData.end()); // Remove name and ':' from vector
            } else {
              // Is the name postfixed
              name = argumentData.back().data;
              argumentData.pop_back(); // Remove name from vector
            }
            if (argumentData.size() > 0) typeList = tokenToStringTypeList(argumentData);
            if (typeList.size() == 0) typeList = {"define"}; // If an argument has no types specified, it can hold any type
            args->push_back({name, new Variable(nullptr, typeList)});
          } else throw Error("Unexpected token in declaration of function " + toks[2].data, "SyntaxError", toks[2].line);
        }
        // Parse return types
        if (toks[pos].data == "=>") {
          pos++; // Ignore the '=>'
          std::vector<Token> returnTypesTokens;
          while (toks[pos].data != "do") {
            returnTypesTokens.push_back(toks[pos]);
            pos++;
            if (pos >= toks.size()) throw Error("Missing block begin 'do' in declaration of function " + toks[2].data, "SyntaxError", toks[2].line);
          }
          returnTypes = tokenToStringTypeList(returnTypesTokens);
          if (returnTypes.size() == 0) throw Error("Missing types in return type specification", "SyntaxError", toks[2].line);
        }
        auto fNode = new FunctionNode(toks[2].data, args, returnTypes);
        fNode->setLineNumber(toks[2].line);
        addToBlock(fNode);
        blockStack.push_back(fNode);
      }
    }
    
    void buildTree(std::vector<Token> tokens) {
      static std::vector<ASTNode*> blockStack {};
      auto addToBlock = [this](ASTNode* child) {
        if (blockStack.size() == 0) tree.addRootChild(child);
        else blockStack.back()->addChild(child);
      };
      auto logicalLines = splitVector(tokens, isNewLine);
      for (uint64 i = 0; i < logicalLines.size(); i++) {
        std::vector<Token>& toks = logicalLines[i];
        if (toks.size() == 0) continue; // Empty line
        if (toks[0].type == TYPE) {
          resolveTypeStatements(toks, addToBlock);
        } else if (toks[0].data == "define" && toks[0].type == KEYWORD) {
          resolveDefineStatements(toks, addToBlock, blockStack);
        } else if (toks[0].data == "while" && toks[0].type == KEYWORD) {
          auto wBlock = new WhileNode(evaluateCondition(toks), new BlockNode());
          wBlock->setLineNumber(toks[0].line);
          if (PARSER_PRINT_WHILE_TREE) wBlock->printTree(blockStack.size());
          addToBlock(wBlock);
          blockStack.push_back(wBlock);
        } else if (toks[0].data == "if" && toks[0].type == KEYWORD) {
          auto cBlock = new ConditionalNode(evaluateCondition(toks), new BlockNode(), new BlockNode());
          cBlock->setLineNumber(toks[0].line);
          if (PARSER_PRINT_COND_TREE) cBlock->printTree(blockStack.size());
          addToBlock(cBlock);
          blockStack.push_back(cBlock);
        } else if (toks[0].data == "else" && toks[0].type == CONSTRUCT) {
          auto cNode = dynamic_cast<ConditionalNode*>(blockStack.back());
          if (cNode == nullptr) throw Error("Cannot find conditional structure for token `else`", "SyntaxError", blockStack.back()->getLineNumber());
          cNode->nextBlock();
        } else if (toks[0].data == "do" && toks[0].type == CONSTRUCT) {
          auto newBlock = new BlockNode();
          addToBlock(newBlock);
          blockStack.push_back(newBlock);
        } else if (toks[0].data == "end" && toks[0].type == CONSTRUCT) {
          blockStack.pop_back();
        } else {
          auto expr = buildExpression(toks);
          if (PARSER_PRINT_EXPR_TREE) expr->printTree(blockStack.size());
          addToBlock(expr);
        }
      }
    }
    
    std::vector<Token> getTokens() {
      return tokens;
    }
    
  };
  
  class Interpreter {
  private:
    AST tree;
  public:
    Interpreter(AST tree): tree(tree) {
      interpret(tree.getRootChildren());
    }
  private:
    void interpret(ChildrenNodes nodes) {
      for (uint64 i = 0; i < nodes.size(); ++i) {
        auto nodeType = nodes[i]->getNodeType();
        if (nodeType == "ExpressionNode") {
          interpretExpression(dynamic_cast<ExpressionNode*>(nodes[i])->getChild());
        } else if (nodeType == "DeclarationNode") {
          registerDeclaration(dynamic_cast<DeclarationNode*>(nodes[i]));
        } else if (nodeType == "ConditionalNode") {
          resolveCondition(dynamic_cast<ConditionalNode*>(nodes[i]));
        } else if (nodeType == "WhileNode") {
          doWhileLoop(dynamic_cast<WhileNode*>(nodes[i]));
        } else if (nodeType == "FunctionNode") {
          FunctionNode* n = dynamic_cast<FunctionNode*>(nodes[i]);
          // Create a fake decl node so name resolution can find it
          auto pseudoDecl = new DeclarationNode({"Function"}, Token(n->getName(), VARIABLE, PHONY_TOKEN));
          // Make it look like it's in the place of the FunctionNode
          pseudoDecl->setParent(n->getParent());
          pseudoDecl->setLineNumber(n->getLineNumber());
          // Add it to the scope
          registerDeclaration(pseudoDecl);
          // Assign the Function object to the newly created declaration, so it can be executed
          pseudoDecl->getParentScope()->at(n->getName())->assign(new Function(n));
        }
      }
    }
    
    void doWhileLoop(WhileNode* node) {
      while (true) {
        auto condition = interpretExpression(node->getCondition()->getChild());
        if (!static_cast<Object*>(condition->t.typeData)->isTruthy()) break;
        interpret(node->getLoopNode()->getChildren());
      }
    }
    
    void resolveCondition(ConditionalNode* node) {
      auto condRes = interpretExpression(node->getCondition()->getChild());
      if (static_cast<Object*>(condRes->t.typeData)->isTruthy()) {
        interpret(node->getTrueBlock()->getChildren());
      } else {
        interpret(node->getFalseBlock()->getChildren());
      }
    }
    
    void registerDeclaration(DeclarationNode* node) {
      if (!contains(std::string("define"), node->typeNames)) {
        // Check that types exist
        for (auto typeName : node->typeNames) {
          auto notDefInThisScope = Error("Type " + typeName + " was not defined in this scope", "NullPointerError", node->getLineNumber());
          auto typeVar = resolveNameFrom(node, typeName);
          if (typeVar == nullptr) throw notDefInThisScope;
          Type* type = dynamic_cast<Type*>(typeVar->read());
          if (type == nullptr) throw notDefInThisScope;
        }
      }
      node->getParentScope()->insert({node->identifier.data, new Variable(nullptr, node->typeNames)});
      if (node->getChildren().size() == 1) {
        Variable* variable = (*node->getParentScope())[node->identifier.data];
        Object* toAssign = static_cast<Object*>(interpretExpression(dynamic_cast<ExpressionNode*>(node->getChild())->getChild())->t.typeData);
        variable->assign(toAssign);
      }
    }
    
    Arguments* parseArgumentsTree(Function* f, ExpressionChildNode* node) {
      Arguments* args = f->getFNode()->getArguments();
      if (node->t.type == OPERATOR && node->t.data == ",") {
        ExpressionChildNode lastNode = *node;
        std::vector<Object*> listOfArgs;
        while (lastNode.t.data == ",") {
          listOfArgs.push_back(static_cast<Object*>(interpretExpression(lastNode[0])->t.typeData));
          lastNode = *(lastNode[1]);
          // This catches the last argument, because the last comma has two leafs and no branch
          if (lastNode.t.data != ",") listOfArgs.push_back(static_cast<Object*>(interpretExpression(&lastNode)->t.typeData));
        }
        std::reverse(listOfArgs.begin(), listOfArgs.end());
        std::size_t pos = 0;
        for (auto arg : listOfArgs) {
          // Don't care if there's more args than necesary, just ignore them:
          // TODO: varargs functions
          if (pos >= args->size()) break;
          args->at(pos).second->assign(arg);
          pos++;
        }
      } else {
        args->at(0).second->assign(static_cast<Object*>(interpretExpression(node)->t.typeData));
      }
      return args;
    }
    
    // If no arguments were provided
    Arguments* parseArgumentsTree(Function* f) {
      return f->getFNode()->getArguments();
    }
    
    ExpressionChildNode* runFunction(ExpressionChildNode* node) {
      if (node->getChildren().size() == 0) throw std::runtime_error("Function call without caller");
      ExpressionChildNode functionData = *((*node)[-1]);
      // Try to get 'this' context
      Object* thisObject = nullptr;
      // Find function object
      Object* wannabeFunction = nullptr;
      if (functionData.t.type == OPERATOR) {
        thisObject = static_cast<Object*>(interpretExpression(functionData[-1])->t.typeData);
        Variable* funVar = dynamic_cast<Variable*>(static_cast<Object*>(interpretExpression(&functionData)->t.typeData));
        if (funVar == nullptr) throw Error("Variable is empty or undefined", "NullPointerError", node->t.line);
        wannabeFunction = funVar->read();
      } else if (functionData.t.type == UNPROCESSED) {
        thisObject = nullptr; // Has no caller
        auto name = functionData.t.data;
        Variable* funVar = resolveNameFrom(node, name);
        if (funVar == nullptr) throw Error("Function " + name + " was not declared in this scope", "NullPointerError", node->t.line);
        wannabeFunction = funVar->read();
      }
      Function* func = dynamic_cast<Function*>(wannabeFunction);
      if (func == nullptr) throw Error("Attempt to call a variable that is not a function", "TypeError", node->t.line);
      // Parse arguments
      Arguments* currentArgs;
      if (node->getChildren().size() >= 2) currentArgs = parseArgumentsTree(func, (*node)[0]);
      else currentArgs = parseArgumentsTree(func);
      // Setup scope
      BlockNode* functionScope = new BlockNode();
      functionScope->setParent(node);
      functionScope->setLineNumber(node->t.line);
      functionScope->getScope()->insert({"this", new Variable(thisObject, {})});
      for (auto argPair : *currentArgs) functionScope->getScope()->insert(argPair);
      // Prepare code
      BlockNode* functionCode = dynamic_cast<BlockNode*>(func->getFNode()->getChildren()[0]);
      functionCode->setParent(functionScope);
      functionCode->setLineNumber(node->t.line);
      // Execute code
      if (functionCode->getNodeType() == "NativeBlockNode") {
        // Native functions
        Object* functionResult = dynamic_cast<NativeBlockNode*>(functionCode)->run(functionScope);
        if (functionResult == nullptr) return nullptr;
        else return new ExpressionChildNode(Token(functionResult, UNPROCESSED, PHONY_TOKEN));
      } else {
        // Normal functions
        interpret(functionCode->getChildren());
        // TODO: handle return statement. assign value somewhere in the scope, then extract it here
        return nullptr; // This should return whatever the `functionCode` above returns ^^
      }
    }
    
    ExpressionChildNode* interpretExpression(ExpressionChildNode* node) {
      if (node->t.type == OPERATOR) {
        if (node->t.data == "()") return runFunction(node);
        ExpressionChildNode* processed = new ExpressionChildNode(node->t);
        processed->setParent(node->getParent());
        auto ch = node->getChildren();
        std::for_each(ch.begin(), ch.end(), [=](ASTNode*& n) {
          auto node = dynamic_cast<ExpressionChildNode*>(n);
          if (node->getChildren().size() != 0) processed->addChild(interpretExpression(node));
          else processed->addChild(node);
        });
        return new ExpressionChildNode(Token(runOperator(processed), UNPROCESSED, PHONY_TOKEN));
      } else if (node->t.type == VARIABLE) {
        Variable* var = resolveNameFrom(node, node->t.data);
        if (var == nullptr) throw Error("Variable " + node->t.data + " was not declared in this scope", "NullPointerError", node->t.line);
        return new ExpressionChildNode(Token(var, VARIABLE, PHONY_TOKEN));
      }
      if (node->getChildren().size() == 0) return node;
      throw std::runtime_error("Wrong type of token.\n");
    }
    
  };
  
} /* namespace lang */

static bool exitCodes = false;
static bool noInterpret = false;
#define EXIT(code) (exitCodes ? code : 0)

int interpretCode() {
  try {
    lang::Parser a(INPUT);
    if (!noInterpret) lang::Interpreter in(a.tree);
  } catch(Error& e) {
    print(e.toString());
    return EXIT(ERROR_CODE_FAILED);
  } catch(std::runtime_error& re) {
    print(re.what(), "\n");
    return EXIT(ERROR_INTERNAL);
  }
  return 0;
}

int main(int argc, char** argv) {
  namespace po = boost::program_options;
  po::options_description desc("Allowed options");
  desc.add_options()
    ("help,h", "display this help")
    ("use-existing,c", "use constants.data and inputs.data")
    ("evaluate,e", po::value<std::string>(), "use to evaluate code from the command line")
    ("read-file,f", po::value<std::string>(), "use to evaluate code from a file at the specified path")
    ("no-interpret,n", "don't interpret the parsed code")
    ("exit-codes", "use to send non-0 exit codes on failure")
  ;
  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);
  
  if (vm.count("help")) {
    print(desc, "\n");
    return 0;
  }
  if (vm.count("exit-codes")) {
    exitCodes = true;
  }
  if (vm.count("no-interpret")) {
    noInterpret = true;
  }
  if (vm.count("use-existing")) {
    getConstants();
    return interpretCode();
  } else if (vm.count("evaluate")) {
    INPUT = vm["evaluate"].as<std::string>();
    return interpretCode();
  } else if (vm.count("read-file")) {
    std::ifstream in(vm["read-file"].as<std::string>());
    std::string contents((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    INPUT = contents;
    return interpretCode();
  } else {
    print(desc, "\n");
    return EXIT(ERROR_BAD_INPUT);
  }
  return 0;
}
