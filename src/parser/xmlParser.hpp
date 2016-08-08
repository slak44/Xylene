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
    } else if (name == "return") {
      auto retNode = Node<ReturnNode>::make();
      auto val = node->first_node("expr");
      if (val != 0) retNode->setValue(Node<ExpressionNode>::dynPtrCast(parseXMLNode(val)));
      return retNode;
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
      auto dynAttr = node->first_attribute("dynamic");
      bool dynamic = dynAttr != 0 && dynAttr->value() == std::string("true");
      if (dynamic) {
        decl = Node<DeclarationNode>::make(ident, TypeList {});
      } else {
        std::string tlValue = node->first_attribute("types")->value();
        auto vec = split(tlValue, ' ');
        decl = Node<DeclarationNode>::make(ident, TypeList(ALL(vec)));
      }
      auto expr = node->first_node("expr");
      if (expr != 0) {
        decl->setInit(Node<ExpressionNode>::dynPtrCast(parseXMLNode(expr)));
      }
      return decl;
    } else if (name == "branch") {
      auto branch = Node<BranchNode>::make();
      auto cond = node->first_node();
      if (cond == 0) throw XMLParseError("Missing condition in branch", {METADATA_PAIRS});
      branch->setCondition(Node<ExpressionNode>::dynPtrCast(parseXMLNode(cond)));
      auto success = cond->next_sibling();
      if (success == 0) throw XMLParseError("Missing success node in branch", {METADATA_PAIRS});
      branch->setSuccessBlock(Node<BlockNode>::dynPtrCast(parseXMLNode(success)));
      auto failiure = success->next_sibling();
      if (failiure != 0) {
        branch->setFailiureBlock(Node<BlockNode>::dynPtrCast(parseXMLNode(failiure)));
      }
      return branch;
    } else if (name == "loop") {
      auto loop = Node<LoopNode>::make();
      auto init = node->first_node("loop_init");
      if (init != 0) {
        loop->setInit(Node<DeclarationNode>::dynPtrCast(parseXMLNode(init)));
      }
      auto cond = node->first_node("loop_condition");
      if (cond != 0) {
        loop->setCondition(Node<ExpressionNode>::dynPtrCast(parseXMLNode(cond)));
      }
      auto update = node->first_node("loop_update");
      if (update != 0) {
        loop->setUpdate(Node<ExpressionNode>::dynPtrCast(parseXMLNode(update)));
      }
      auto code = node->first_node("block");
      if (code != 0) {
        loop->setCode(Node<BlockNode>::dynPtrCast(parseXMLNode(code)));
      }
      return loop;
    } else if (name == "loop_init" || name == "loop_condition" || name == "loop_update") {
      return parseXMLNode(node->first_node());
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
