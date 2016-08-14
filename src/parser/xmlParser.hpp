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

/*
  XML files should contain a single root tag. The root tag must represent a block node.
  List of tags:
  - block: no attributes, maps to a BlockNode
  - return: no attributes, has 0 or 1 expr tags as his children, maps to a ReturnNode
  - expr: maps to an ExpressionNode
    - attribute type: is a string name for a TokenType
    - attribute value: equivalent to Token's data field (for Operators it is the string name)
    - children: can be other expr nodes, as many as the operator's arity
  - decl: maps to a DeclarationNode
    - attribute ident: name of declared thing
    - attribute dynamic: if equal to "true", declaration has dynamic type
    - attribute types: space-separated list of acceptable types (eg "Integer Float"). Is a no-op if dynamic is true
    - children: only one expr node
  - branch: maps to a BranchNode
    - children: an expr node that is the condition, a success block, and either a failiure block or another branch node
  - loop: maps to a LoopNode
    - children: one or none of each node: loop_init, loop_condition, loop_update. A block is mandatory
  - loop_init: only inside a loop tag
    - children: one expr node
  - loop_condition: only inside a loop tag
    - children: one expr node
  - loop_update: only inside a loop tag
    - children: one expr node
*/

class XMLParser: public BaseParser {
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
    auto rootNode = Node<BlockNode>::dynPtrCast(parseXMLNode(doc.first_node()));
    if (rootNode == 0) throw XMLParseError("Root node isn't a block node", {
      METADATA_PAIRS,
      {"tag name", doc.first_node()->name()}
    });
    tree.setRoot(*rootNode);
  }
  
  void parse(rapidxml::file<> xmlFile) {
    parse(xmlFile.data());
  }
  
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
        content = PtrUtil<Token>::unique(tokenType, operatorIndexFrom(data), 0);
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
      auto blockFailiure = success->next_sibling("block");
      if (blockFailiure != 0) {
        branch->setFailiureBlock(Node<BlockNode>::dynPtrCast(parseXMLNode(blockFailiure)));
      }
      auto branchFailiure = success->next_sibling("branch");
      if (branchFailiure != 0) {
        branch->setFailiureBlock(Node<BranchNode>::dynPtrCast(parseXMLNode(branchFailiure)));
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
};

#endif
