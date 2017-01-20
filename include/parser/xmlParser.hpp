#ifndef XML_PARSER_HPP
#define XML_PARSER_HPP

#include <string>
#include <rapidxml.hpp>
#include <rapidxml_utils.hpp>

#include "utils/util.hpp"
#include "utils/error.hpp"
#include "utils/typeInfo.hpp"
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
    - attribute \b type: "root", "if", "code", "function" defaults to "code" if not specified
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
  - \c function: maps to a FunctionNode
    - attribute \b ident: name of function, optional
    - attribute \b return: space separated list of return types, omit for void
    - attribute \b args: comma separated list of arguments. An argument is a identifier, a colon, then a space separated list of types for it
    - attribute \b foreign: true or false. If true, this has no children and is an externally defined func
    - children: zero or one block node
  - \c type: maps to a TypeNode
    - attribute \b name: name of new type
    - attribute \b inherits: list of space space-separated superclasses
    - children: any number of constructors, methods and members
  - \c constructor: maps to a ConstructorNode
    - attribute \b args: comma separated list of arguments. An argument is a identifier, a colon, then a space separated list of types for it
    - attribute \b visibility: public, private or protected
    - children: inherit from function
  - \c method: maps to a MethodNode
    - inherit function attributes and children
    - attribute \b visibility: public, private or protected
    - attribute \b static: true if static
  - \c member: maps to a MemberNode
    - inherits declaration attributes and children
    - attribute \b visibility: public, private or protected
    - attribute \b static: true if static
*/
class XMLParser {
private:
  /// Implementation detail. Parses all the children of an XML node
  void parseChildren(rapidxml::xml_node<>* node, ASTNode::Link target);
  /// Implementation detail. Recursively parse XML nodes
  ASTNode::Link parseXMLNode(rapidxml::xml_node<>* node);
  
  inline XMLParser() {}
public:
  /// Parse an XML string
  static AST parse(char* str);
  /// Parse an XML file
  static AST parse(rapidxml::file<char> xmlFile);
};

#endif
