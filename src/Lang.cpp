#include <iostream>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <algorithm>
#include <functional>
#include <unordered_map>

#include "global.hpp"
#include "operators.hpp"
#include "tokens.hpp"
#include "nodes.hpp"
#include "builtins.hpp"
#include "operator_maps.hpp"

namespace lang {
  
  class Parser {
  private:
    std::vector<Token> tokens {};
    inline void skipCharacters(unsigned int& i, int by) {i += by;}
    // If we're already at the next character, but the loop will increment i by 1
    inline void preventIncrement(unsigned int& i) {i--;}
    
    std::vector<Token> variables {};
    std::vector<std::string> keywords {"define", "if", "while"};
    std::vector<std::string> constructKeywords {"do", "end", "else"};
  public:
    AST tree = AST();
    
    Parser(std::string code) {
      if (PARSER_PRINT_INPUT) print(code, "\n");
      try {
        tokenize(code);
        if (PARSER_PRINT_TOKENS) for (auto tok : tokens) print(tok, "\n");
        if (PARSER_PRINT_AS_EXPR) for (auto tok : ExpressionNode(tokens).getRPNOutput()) print(tok.data, " ");
        buildTree(tokens);
      } catch (SyntaxError &e) {
        print(e.toString(), "\n");
      }
    }
    
    void tokenize(std::string code) {
      unsigned int lines = 0;
      for (unsigned int i = 0; i < code.length(); ++i) {
        if (code[i] == '/' && code[i + 1] == '/') while(code[i] != '\n' && code[i] != '\0') skipCharacters(i, 1);
        if (code[i] == '\n') lines++;
        
        // Ignore whitespace
        if (isspace(code[i])) continue;
        
        // Block begin/end
        // Parenthesis
        // Semicolons
        if (code[i] == '{' || code[i] == '}' ||
        code[i] == '(' || code[i] == ')' ||
        code[i] == ';') {
          tokens.push_back(Token(std::string(1, code[i]), CONSTRUCT, lines));
          continue;
        }
        
        // Operators
        auto initTokSize = tokens.size();
        std::for_each(opList.begin(), opList.end(), [this, initTokSize, code, i, lines](Operator& op) {
          if (initTokSize < tokens.size()) return; // If there are more tokens than before for_each, the operator was already added, so return.
          if (op.toString().length() + i > code.length()) return; // If the operator is longer than the source string, ignore it.
          if (op.toString() == code.substr(i, op.toString().length())) {
            Operator* tmp = &op;
            // TODO: apply DRY on these ifs
            if (tmp->toString() == "++" || tmp->toString() == "--") {
              // Prefix version
              if (tokens.back().type == OPERATOR || tokens.back().type == CONSTRUCT) tmp = new Operator(tmp->toString(), 12, ASSOCIATE_FROM_RIGHT, UNARY);
              // Postfix version
              else tmp = new Operator(tmp->toString(), 13, ASSOCIATE_FROM_RIGHT, UNARY);
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
              if (isFloat) throw SyntaxError("Malformed float, multiple decimal points: \"" + current + "\"", lines);
              isFloat = true;
            }
            skipCharacters(i, 1);
          }
          preventIncrement(i);
          if (current[current.length() - 1] == '.') throw SyntaxError("Malformed float, missing digits after decimal point: \"" + current + "\"", lines);
          if (base != 10 && isFloat) throw SyntaxError("Floating point numbers must be used with base 10 numbers: \"" + current + "\"", lines);
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
            // TODO: add escape sequences
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
    
    void buildTree(std::vector<Token> tokens) {
      static std::vector<BlockNode*> blockStack {};
      auto addToBlock = [this](ASTNode* child) {
        if (blockStack.size() == 0) tree.addRootChild(child);
        else blockStack.back()->addChild(child);
      };
      auto logicalLines = splitVector(tokens, isNewLine);
      for (uint64 i = 0; i < logicalLines.size(); i++) {
        std::vector<Token>& toks = logicalLines[i];
        if (toks.size() == 0) continue;
        // TODO: check for solid types here as well
        if (toks[0].data == "define" && toks[0].type == KEYWORD) {
          DeclarationNode* decl = new DeclarationNode("define", toks[1]);
          decl->setLineNumber(toks[1].line);
          if (toks[2].data != ";" || toks[2].type != CONSTRUCT) {
            std::vector<Token> exprToks(toks.begin() + 1, toks.end());
            ExpressionNode* expr = new ExpressionNode(exprToks);
            expr->buildSubtree();
            decl->addChild(expr);
          }
          if (PARSER_PRINT_DECL_TREE) decl->printTree(blockStack.size());
          addToBlock(decl);
        } else if (toks[0].data == "while" && toks[0].type == KEYWORD) {
          auto wBlock = new WhileNode(evaluateCondition(toks), new BlockNode());
          if (PARSER_PRINT_WHILE_TREE) wBlock->printTree(blockStack.size());
          addToBlock(wBlock);
          blockStack.push_back(wBlock);
        } else if (toks[0].data == "if" && toks[0].type == KEYWORD) {
          auto cBlock = new ConditionalNode(evaluateCondition(toks), new BlockNode(), new BlockNode());
          if (PARSER_PRINT_COND_TREE) cBlock->printTree(blockStack.size());
          addToBlock(cBlock);
          blockStack.push_back(cBlock);
        } else if (toks[0].data == "else" && toks[0].type == CONSTRUCT) {
          auto cNode = dynamic_cast<ConditionalNode*>(blockStack.back());
          if (cNode == nullptr) throw SyntaxError("Cannot find conditional structure for token `else`.\n", blockStack.back()->getLineNumber());
          cNode->nextBlock();
        } else if (toks[0].data == "end" && toks[0].type == CONSTRUCT) {
          blockStack.pop_back();
        } else {
          ExpressionNode* expr = new ExpressionNode(toks);
          expr->buildSubtree();
          expr->setLineNumber(dynamic_cast<ExpressionChildNode*>(expr->getChildren()[0])->t.line);
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
          interpretExpression(dynamic_cast<ExpressionChildNode*>(nodes[i]->getChildren()[0]))->printTree(0);
        } else if (nodeType == "DeclarationNode") {
          registerDeclaration(dynamic_cast<DeclarationNode*>(nodes[i]));
        } else if (nodeType == "ConditionalNode") {
          resolveCondition(dynamic_cast<ConditionalNode*>(nodes[i]));
        } else if (nodeType == "WhileNode") {
          doWhileLoop(dynamic_cast<WhileNode*>(nodes[i]));
        }
      }
    }
    
    void doWhileLoop(WhileNode* node) {
      while (true) {
        auto condition = interpretExpression(dynamic_cast<ExpressionChildNode*>(node->getCondition()->getChild()));
        if (!static_cast<Object*>(condition->t.typeData)->isTruthy()) break;
        interpret(node->getLoopNode()->getChildren());
      }
    }
    
    void resolveCondition(ConditionalNode* node) {
      auto condRes = interpretExpression(dynamic_cast<ExpressionChildNode*>(node->getCondition()->getChild()));
      if (static_cast<Object*>(condRes->t.typeData)->isTruthy()) {
        interpret(node->getTrueBlock()->getChildren());
      } else {
        interpret(node->getFalseBlock()->getChildren());
      }
    }
    
    void registerDeclaration(DeclarationNode* node) {
      node->getParentScope()->insert({node->identifier.data, new Variable()});
      if (node->getChildren().size() == 1) {
        (*node->getParentScope())[node->identifier.data]->assign(
          static_cast<Object*>(interpretExpression(dynamic_cast<ExpressionChildNode*>(node->getChild()->getChildren()[0]))->t.typeData)
        );
      }
    }
    
    ExpressionChildNode* interpretExpression(ExpressionChildNode* node) {
      if (node->getChildren().size() == 0) return node;
      if (node->t.type == OPERATOR) {
        ExpressionChildNode* processed = new ExpressionChildNode(node->t);
        processed->setParent(node->getParent());
        auto ch = node->getChildren();
        std::for_each(ch.begin(), ch.end(), [=](ASTNode*& n) {
          auto node = dynamic_cast<ExpressionChildNode*>(n);
          if (node->getChildren().size() != 0) processed->addChild(interpretExpression(node));
          else processed->addChild(node);
        });
        return new ExpressionChildNode(Token(runOperator(processed), UNPROCESSED, -2));
      }
      throw std::runtime_error("Wrong type of token.\n");
    }
    
  };
  
} /* namespace lang */

void printHelp() {
  print(
    "Use -c to read from constants.data and inputs.data, -e [CODE] to evaluate code and -f [PATH] to load code from a file.\n",
    "Examples:\n",
    "  lang -c\n",
    "  lang -e \"if true != false do 1 + 1; end\"\n",
    "  lang -f \"/path/to/file.txt\"\n"
  );
}

int main(int argc, char** argv) {
  if (argc == 1) {
    printHelp();
    return 1;
  }
  else if (std::string(argv[1]) == "-c") getConstants();
  else if (std::string(argv[1]) == "-e") INPUT = argv[2];
  else if (std::string(argv[1]) == "-f") {
    std::ifstream in(argv[2]);
    std::string contents((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    INPUT = contents;
  } else {
    printHelp();
    return 1;
  }
  try {
    lang::Parser a(INPUT);
    lang::Interpreter in(a.tree);
  } catch(SyntaxError& se) {
    print(se.toString(), "\n");
  } catch(TypeError& te) {
    print(te.toString(), "\n");
  }
  return 0;
}
