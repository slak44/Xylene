#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <functional>
#include <unordered_map>
#include <utility>
#include <typeinfo>

#include "std_is_missing_stuff.hpp"
#include "global.hpp"
#include "operators.hpp"
#include "tokens.hpp"
#include "nodes.hpp"
#include "builtins.hpp"


class Parser {
private:
  std::vector<Token> tokens {};
  inline void skipCharacters(unsigned int& i, int by) {i += by;}
  // If we're already at the next character, but the loop will increment i by 1
  inline void preventIncrement(unsigned int& i) {i--;}

  std::vector<std::string> typeIdentifiers {"Integer"}; // TODO: add this
  std::vector<std::string> keywords {"define", "if", "while"};
public:
  nodes::AST tree = nodes::AST();

  Parser(std::string code) {
    if (PARSER_PRINT_INPUT) print(code, "\n");
    try {
      tokenize(code);
      buildTree();
      if (PARSER_PRINT_TOKENS) for (auto tok : tokens) print(tok, "\n");
      if (PARSER_PRINT_AS_EXPR) for (auto tok : nodes::ExpressionNode(tokens).getRPNOutput()) print(tok.data, " ");
    } catch (SyntaxError &e) {
      print(e.getMessage(), "\n");
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
        tokens.push_back(Token(to_string(code[i]), CONSTRUCT, lines));
        continue;
      }
      
      // Operators
      auto initTokSize = tokens.size();
      std::for_each(ops::opList.begin(), ops::opList.end(), [this, initTokSize, code, i, lines](ops::Operator& op) {
        if (initTokSize < tokens.size()) return; // If there are more tokens than before for_each, the operator was already added, so return.
        if (op.getName().length() + i > code.length()) return; // If the operator is longer than the source string, ignore it.
        if (op.getName() == code.substr(i, op.getName().length())) {
          ops::Operator* tmp = &op;
          // TODO: apply DRY on these ifs
          if (tmp->getName() == "++" || tmp->getName() == "--") {
            // Prefix version
            if (tokens.back().type == OPERATOR || tokens.back().type == CONSTRUCT) tmp = new ops::Operator(tmp->getName(), 12, ops::ASSOCIATE_FROM_RIGHT, ops::UNARY);
            // Postfix version
            else tmp = new ops::Operator(tmp->getName(), 13, ops::ASSOCIATE_FROM_RIGHT, ops::UNARY);
          }
          if (tmp->getName() == "+" || tmp->getName() == "-") {
            // Unary version
            if (tokens.back().type == OPERATOR || tokens.back().type == CONSTRUCT) tmp = new ops::Operator(tmp->getName(), 12, ops::ASSOCIATE_FROM_RIGHT, ops::UNARY);
            // Binary version
            else tmp = new ops::Operator(tmp->getName(), 10);
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
            case 'b': base = 2;  skipCharacters(i, 2); break;
            case 'o': base = 8;  skipCharacters(i, 2); break;
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
        if (isFloat) t = Token(new builtins::Float(current), FLOAT, lines);
        else t = Token(new builtins::Integer(current, base), INTEGER, lines);
        tokens.push_back(t);//TODO here go to exprnode and things then interpreter
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
        tokens.push_back(Token(current, STRING, lines));
        continue;
      }

      // Others
      Token token = Token();
      while (!isspace(code[i])) {
        if (ops::isReservedChar(code[i]) || code[i] == '\0') break;
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
      if (toks[0].data == "define" && toks[0].type == KEYWORD) {
        toks[1].type = VARIABLE;
        nodes::DeclarationNode* decl = new nodes::DeclarationNode("define", toks[1]);
        if (toks[2].data != ";" || toks[2].type != CONSTRUCT) {
          nodes::ExpressionNode* expr = new nodes::ExpressionNode(toks);
          expr->buildSubtree();
          decl->addChild(expr);
        }
        decl->printTree(0);
        tree.addRootChild(decl);
      } else {
        nodes::ExpressionNode* expr = new nodes::ExpressionNode(toks);
        expr->buildSubtree();
        expr->printTree(0);
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
  typedef builtins::Object Object;
  nodes::AST tree;
  std::unordered_map<std::string, nodes::DeclarationNode*> variables();
public:
  Interpreter(nodes::AST tree): tree(tree) {
    interpret();
  }
private:
  void interpret() {
    nodes::ChildrenNodes nodes = tree.getRootChildren();
    for (uint64 i = 0; i < nodes.size(); ++i) {
//      if (nodes[i]->getNodeType() == "ExpressionNode") interpretExpression(dynamic_cast<nodes::ExpressionChildNode*>(nodes[i]->getChildren()[0]));
    }
  }
  
  nodes::ExpressionChildNode* interpretExpression(nodes::ExpressionChildNode* node) {
    if (node->getChildren().size() == 0) return node;
    if (node->t.type == OPERATOR) {
      ops::Operator* op = static_cast<ops::Operator*>(node->t.typeData);
      auto ch = node->getChildren();
      std::for_each(ch.begin(), ch.end(), [this](nodes::ASTNode*& n) {
        auto node = dynamic_cast<nodes::ExpressionChildNode*>(n);
        if (node->getChildren().size() != 0) n = interpretExpression(node);
      });
      if (dynamic_cast<nodes::ExpressionChildNode*>(ch.back())->t.type == INTEGER) {
//        if (op->getArity() == ops::BINARY)
//          static_cast<std::function<builtins::Integer*(builtins::Integer, builtins::Integer)> >(builtins::Integer::operators[op])(ch.back(), ch[ch.size() - 2]);
      }
    }
    return nullptr;
  }
  
};

int main() {
  getConstants();
  Parser* a;
  switch (TEST_INPUT) {
    case 1: a = new Parser("6 >> 4 >> 2 * 1"); break;
    case 2: a = new Parser("(1 + 2) * 3 / (2 << 1)"); break; 
    case 3: a = new Parser("define a = \"abc123\";"); break;
    case 4: a = new Parser("define a = 132;\na+=1+123*(1 + 32/2);"); break;
    case 5: a = new Parser("1+ a*(-19-1++)==Integer.MAX_INT"); break;
    case 6: a = new Parser("define a = 1 + 2 * (76 - 123 - (43 + 12) / 5) % 10;\nInteger n = 1;"); break;
    case 7: a = new Parser("1 + 2 * 3 << 2"); break;
    case 8: a = new Parser("Test.test.abc.df23.asdasf ()"); break;
    case 9: a = new Parser("define a = 1 + 2;\nb = 2 * 3"); break;
    default: break;
  }
  Interpreter in(a->tree);

  return 0;
}
