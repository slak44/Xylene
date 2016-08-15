#ifndef XML_PARSER_HPP
#define XML_PARSER_HPP

#include <string>
#include <rapidxml_utils.hpp>

#include "utils/util.hpp"
#include "utils/error.hpp"
#include "baseParser.hpp"
#include "ast.hpp"

class XMLParseError: public InternalError {
public:
  XMLParseError(std::string msg, ErrorData data);
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
  XMLParser();
  
  void parse(char* str);
  void parse(rapidxml::file<> xmlFile);
  
private:
  void parseChildren(rapidxml::xml_node<>* node, ASTNode::Link target);
  ASTNode::Link parseXMLNode(rapidxml::xml_node<>* node);
};

#endif
