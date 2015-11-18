#include <iostream>
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

namespace lang {
  
  class Parser {
  private:
    std::vector<Token> tokens {};
    inline void skipCharacters(unsigned int& i, int by) {i += by;}
    // If we're already at the next character, but the loop will increment i by 1
    inline void preventIncrement(unsigned int& i) {i--;}
    
    std::vector<std::string> keywords {"define", "if", "while"};
  public:
    AST tree = AST();
    
    Parser(std::string code) {
      if (PARSER_PRINT_INPUT) print(code, "\n");
      try {
        tokenize(code);
        if (PARSER_PRINT_TOKENS) for (auto tok : tokens) print(tok, "\n");
        if (PARSER_PRINT_AS_EXPR) for (auto tok : ExpressionNode(tokens).getRPNOutput()) print(tok.data, " ");
        buildTree();
      } catch (SyntaxError &e) {
        print(e.toString(), "\n");
      }
    }
    
    void tokenize(std::string code) {
      unsigned int lines = 0;
      for (unsigned int i = 0; i < code.length(); ++i) {
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
        // Check if the thing is a keyword
        if (contains(token.data, keywords)) token.type = KEYWORD;
        preventIncrement(i);
        token.line = lines;
        tokens.push_back(token);
      }
    }
    
    void buildTree() {
      std::function<bool(Token)> isNewLine = [](Token tok) {return tok.data == ";" && tok.type == CONSTRUCT;};
      auto logicalLines = splitVector(tokens, isNewLine);
      for (auto toks : logicalLines) {
        if (toks.size() == 0) continue;
        // TODO: check for solid types here as well
        if (toks[0].data == "define" && toks[0].type == KEYWORD) {
          toks[1].type = VARIABLE;
          DeclarationNode* decl = new DeclarationNode("define", toks[1]);
          decl->setLineNumber(toks[1].line);
          if (toks[2].data != ";" || toks[2].type != CONSTRUCT) {
            ExpressionNode* expr = new ExpressionNode(toks);
            expr->buildSubtree();
            decl->addChild(expr);
          }
          if (PARSER_PRINT_DECL_TREE) decl->printTree(0);
          tree.addRootChild(decl);
        } else {
          ExpressionNode* expr = new ExpressionNode(toks);
          expr->buildSubtree();
          expr->setLineNumber(dynamic_cast<ExpressionChildNode*>(expr->getChildren()[0])->t.line);
          if (PARSER_PRINT_EXPR_TREE) expr->printTree(0);
          tree.addRootChild(expr);
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
    std::unordered_map<std::string, Variable*> variables {};
  public:
    Interpreter(AST tree): tree(tree) {
      interpret();
    }
  private:
    void interpret() {
      ChildrenNodes nodes = tree.getRootChildren();
      for (uint64 i = 0; i < nodes.size(); ++i) {
        auto nodeType = nodes[i]->getNodeType();
        if (nodeType == "ExpressionNode") {
          nodes[i]->getChildren()[0] = interpretExpression(dynamic_cast<ExpressionChildNode*>(nodes[i]->getChildren()[0]));
          print("\n\n");
          dynamic_cast<ExpressionChildNode*>(nodes[i]->getChildren()[0])->printTree(0);
        } else if (nodeType == "DeclarationNode") {
          Token child = dynamic_cast<ExpressionChildNode*>(nodes[i]->getChildren()[0]->getChildren()[0])->t;
          if (nodes[i]->getChildren().size() == 0) {
            // TODO declnodes
          }
        }
      }
    }
    
    ExpressionChildNode* interpretExpression(ExpressionChildNode* node) {
      if (node->getChildren().size() == 0) return node;
      if (node->t.type == OPERATOR) {
        Operator* op = static_cast<Operator*>(node->t.typeData);
        auto ch = node->getChildren();
        // TODO: could this be multithreaded?
        std::for_each(ch.begin(), ch.end(), [this](ASTNode*& n) {
          auto node = dynamic_cast<ExpressionChildNode*>(n);
          if (node->getChildren().size() != 0) n = interpretExpression(node);
        });
        auto arity = op->getArity();
        Object* result;
        if (arity == UNARY) {
          Object* operand = static_cast<Object*>(dynamic_cast<ExpressionChildNode*>(ch[0])->t.typeData);
          result = runOperator(op, operand);
        } else if (arity == BINARY) {
          Object* operandLeft = static_cast<Object*>(dynamic_cast<ExpressionChildNode*>(ch[1])->t.typeData);
          Object* operandRight = static_cast<Object*>(dynamic_cast<ExpressionChildNode*>(ch[0])->t.typeData);
          result = runOperator(op, operandLeft, operandRight);
        } else if (arity == TERNARY) {
          Object* operand1 = static_cast<Object*>(dynamic_cast<ExpressionChildNode*>(ch[2])->t.typeData);
          Object* operand2 = static_cast<Object*>(dynamic_cast<ExpressionChildNode*>(ch[1])->t.typeData);
          Object* operand3 = static_cast<Object*>(dynamic_cast<ExpressionChildNode*>(ch[0])->t.typeData);
          result = runOperator(op, operand1, operand2, operand3);
        }
        return new ExpressionChildNode(Token(result, UNPROCESSED, -2));
      }
      return nullptr;
    }
    
  };
  
} /* namespace lang */

int main() {
  getConstants();
  lang::Parser a(INPUT);
  lang::Interpreter in(a.tree);
  return 0;
}
