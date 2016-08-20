#ifndef XML_PARSER_HPP
#define XML_PARSER_HPP

#include <string>
#include <rapidxml.hpp>
#include <rapidxml_utils.hpp>

#include "utils/util.hpp"
#include "utils/error.hpp"
#include "baseParser.hpp"
#include "ast.hpp"

/**
  \brief Thrown when an XML file can't be parsed.
*/
class XMLParseError: public InternalError {
public:
  XMLParseError(std::string msg, ErrorData data);
};

/**
  \brief Creates an AST from an XML file or string.
  
  XML data should contain a single root tag. The root tag must represent a block node.
  List of tags:
  - \c block: maps to a BlockNode
    - attribute \b type: "root", "if", "code", defaults to "code" if not specified
  - \c return: no attributes, has 0 or 1 expr tags as his children, maps to a ReturnNode
  - \c expr: maps to an ExpressionNode
    - attribute \b type: is a string name for a TokenType
    - attribute \b value: equivalent to Token's data field (for Operators it is the string name)
    - children: can be other expr nodes, as many as the operator's arity
  - \c decl: maps to a DeclarationNode
    - attribute \b ident: name of declared thing
    - attribute \b dynamic: if equal to "true", declaration has dynamic type
    - attribute \b types: space-separated list of acceptable types (eg "Integer Float"). Is a no-op if dynamic is true
    - children: only one expr node
  - \c branch: maps to a BranchNode
    - children: an expr node that is the condition, a success block, and either a failiure block or another branch node
  - \c loop: maps to a LoopNode
    - children: one or none of each node: loop_init, loop_condition, loop_update. A block is mandatory
  - \c loop_init: only inside a loop tag
    - children: one expr node
  - \c loop_condition: only inside a loop tag
    - children: one expr node
  - \c loop_update: only inside a loop tag
    - children: one expr node
  - \c break: maps to a BreakLoopNode (no attributes, no children)
*/
class XMLParser: public BaseParser {
public:
  XMLParser();
  
  /// Parse an XML string
  XMLParser& parse(char* str);
  /// Parse an XML file
  XMLParser& parse(rapidxml::file<char> xmlFile);
  
private:
  /// Implementation detail. Parses all the children of an XML node
  void parseChildren(rapidxml::xml_node<>* node, ASTNode::Link target);
  /// Implementation detail. Recursively parse XML nodes
  ASTNode::Link parseXMLNode(rapidxml::xml_node<>* node);
};

#endif
