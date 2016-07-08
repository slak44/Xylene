#ifndef XML_PARSER_HPP
#define XML_PARSER_HPP

#include <string>
#include <rapidxml_utils.hpp>

#include "baseParser.hpp"
#include "ast.hpp"

class XMLParseError: public InternalError {
public:
  XMLParseError(std::string msg, ErrorData data):
    InternalError("XMLParseError", msg, data) {}
};

class XMLParser: public BaseParser {
private:
  void parseChildren(rapidxml::xml_node<>* node, ASTNode::Link target) {
    for (auto child = node->first_node(); child; child = child->next_sibling()) {
      auto parsedChild = parseXMLNode(child);
      target->addChild(parsedChild);
    }
  }
  
  ASTNode::Link parseXMLNode(rapidxml::xml_node<>* node) {
    if (node == nullptr) throw XMLParseError("Null node", {METADATA_PAIRS});
    std::string name = node->name();
    if (name == "block") {
      auto block = Node<BlockNode>::make();
      parseChildren(node, block);
      return block;
    } else if (name == "expr") {
      TokenType tokenType = getTokenTypeByName(node->first_attribute("type")->value());
      std::string data = node->first_attribute("value")->value();
      PtrUtil<Token>::U content;
      if (tokenType == OPERATOR) {
        content = PtrUtil<Token>::unique(tokenType, operatorNameMap[data], 0);
      } else {
        content = PtrUtil<Token>::unique(tokenType, data, 0);
      }
      auto expr = Node<ExpressionNode>::make(*content);
      parseChildren(node, expr);
      return expr;
    } else if (name == "decl") {
      Node<DeclarationNode>::Link decl;
      std::string ident = node->first_attribute("ident")->value();
      bool dynamic = node->first_attribute("dynamic")->value() == std::string("true");
      if (dynamic) {
        decl = Node<DeclarationNode>::make(ident);
      } else {
        std::string tlValue = node->first_attribute("types")->value();
        std::istringstream iss(tlValue);
        auto vec = split(tlValue, ' ');
        decl = Node<DeclarationNode>::make(ident, TypeList(ALL(vec)));
      }
      parseChildren(node, decl);
      return decl;
    } else if (name == "branch") {
      auto branch = Node<BranchNode>::make();
      auto cond = node->first_node();
      branch->addCondition(Node<ExpressionNode>::dynPtrCast(parseXMLNode(cond)));
      auto success = node->next_sibling();
      branch->addSuccessBlock(Node<BlockNode>::dynPtrCast(parseXMLNode(success)));
      auto failiure = node->next_sibling();
      if (failiure != 0) {
        branch->addFailiureBlock(Node<BlockNode>::dynPtrCast(parseXMLNode(failiure)));
      }
      return branch;
    }
    throw XMLParseError("Unknown type of node", {METADATA_PAIRS, {"node name", name}});
  }
public:
  XMLParser() {}
  
  void parse(char* str) {
    rapidxml::xml_document<char> doc;
    try {
      doc.parse<0>(str);
    } catch (rapidxml::parse_error& pe) {
      throw XMLParseError("XML parsing failed", {
        METADATA_PAIRS,
        {"xml error", pe.what()},
        {"position", std::to_string(*pe.where<int>())}
      });
    }
    auto root = Node<BlockNode>::dynPtrCast(parseXMLNode(doc.first_node()));
    tree.setRoot(*root);
  }
  
  void parse(rapidxml::file<> xmlFile) {
    parse(xmlFile.data());
  }
};

#endif
