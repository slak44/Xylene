#include "parser/xmlParser.hpp"

#ifndef __clang__
#ifdef __GNUG__
// Too many false-positives involving default parameters in library functions
#pragma GCC diagnostic ignored "-Wzero-as-null-pointer-constant"
#endif
#endif

XMLParseError::XMLParseError(std::string msg, ErrorData data):
  InternalError("XMLParseError", msg, data) {}

AST XMLParser::parse(char* str) {
  XMLParser xpx = XMLParser();
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
  auto rootNode = Node<BlockNode>::dynPtrCast(xpx.parseXMLNode(doc.first_node()));
  if (rootNode == nullptr) throw XMLParseError("Root node isn't a block node", {
    METADATA_PAIRS,
    {"tag name", doc.first_node()->name()}
  });
  return AST(rootNode);
}

AST XMLParser::parse(rapidxml::file<char> xmlFile) {
  return parse(xmlFile.data());
}

void XMLParser::parseChildren(rapidxml::xml_node<>* node, ASTNode::Link target) {
  for (auto child = node->first_node(); child; child = child->next_sibling()) {
    auto parsedChild = parseXMLNode(child);
    target->addChild(parsedChild);
  }
}

static Visibility fromString(std::string str) {
  Visibility vis =
    str == "private" ? PRIVATE :
    str == "protected" ? PROTECTED :
    str == "public" ? PUBLIC :
      INVALID;
  if (vis == INVALID) throw XMLParseError("Invalid visibility for member", {
    METADATA_PAIRS,
    {"invalid value", str}
  });
  return vis;
}

ASTNode::Link XMLParser::parseXMLNode(rapidxml::xml_node<>* node) {
  auto safeAttr =
  [node](std::string what, std::string defaultValue = "") -> std::string {
    auto attr = node->first_attribute(what.c_str());
    if (attr != nullptr) return attr->value();
    else return defaultValue;
  };
  auto requiredAttr = [node](std::string what) -> std::string {
    auto attr = node->first_attribute(what.c_str());
    if (attr != nullptr) return attr->value();
    else throw XMLParseError("Can't find required attribute", {
      METADATA_PAIRS,
      {"missing attribute", what}
    });
  };
  auto funArgs = [node, &safeAttr]() -> FunctionSignature::Arguments {
    FunctionSignature::Arguments args {};
    std::vector<std::string> stringArgs = split(safeAttr("args"), ',');
    for (auto& arg : stringArgs) {
      std::vector<std::string> namePlusTypes = split(arg, ':');
      std::vector<std::string> types = split(namePlusTypes[1], ' ');
      args.push_back(std::make_pair(namePlusTypes[0], TypeList(ALL(types))));
    }
    return args;
  };
  auto funRet = [node, &safeAttr]() -> TypeInfo {
    std::vector<std::string> returnTypes = split(safeAttr("return"), ' ');
    return returnTypes.size() == 0 ? nullptr : TypeInfo(TypeList(ALL(returnTypes)));
  };
  auto funBlock = [=](Node<FunctionNode>::Link f) {
    if (!f->isForeign()) {
      auto codeBlock = node->first_node("block");
      if (codeBlock == nullptr)
        throw XMLParseError("Method missing code block", {METADATA_PAIRS});
      f->code(Node<BlockNode>::staticPtrCast(parseXMLNode(codeBlock)));
    }
  };
  auto getVisibility = [node, &safeAttr]() -> Visibility {
    return fromString(safeAttr("visibility", "private"));
  };
  auto boolAttr = [node, &safeAttr](std::string what) -> bool {
    return safeAttr(what, "false") == "true";
  };

  if (node == nullptr) throw XMLParseError("Null node", {METADATA_PAIRS});
  std::string name = node->name();
  if (name == "block") {
    std::string type = safeAttr("type", "code");
    BlockType bt =
      type == "root" ? ROOT_BLOCK :
      type == "if" ? IF_BLOCK :
      type == "function" ? FUNCTION_BLOCK : CODE_BLOCK;
    auto block = Node<BlockNode>::make(bt);
    parseChildren(node, block);
    return block;
  } else if (name == "return") {
    auto retNode = Node<ReturnNode>::make();
    auto val = node->first_node("expr");
    if (val != nullptr)
      retNode->value(Node<ExpressionNode>::dynPtrCast(parseXMLNode(val)));
    return retNode;
  } else if (name == "expr") {
    TokenType tokenType = TT::findByPrettyName(requiredAttr("type"));
    std::string data = requiredAttr("value");
    std::unique_ptr<Token> content;
    if (tokenType == TT::OPERATOR) {
      content = std::make_unique<Token>(
        tokenType, Operator::find(data), defaultTrace);
    } else {
      content = std::make_unique<Token>(tokenType, data, defaultTrace);
    }
    auto expr = Node<ExpressionNode>::make(*content);
    parseChildren(node, expr);
    return expr;
  } else if (name == "decl") {
    Node<DeclarationNode>::Link decl;
    std::string ident = requiredAttr("ident");
    bool isDynamic = boolAttr("dynamic");
    if (isDynamic) {
      decl = Node<DeclarationNode>::make(ident, TypeList {});
    } else {
      std::string tlValue = requiredAttr("types");
      auto vec = split(tlValue, ' ');
      decl = Node<DeclarationNode>::make(ident, TypeList(ALL(vec)));
    }
    auto expr = node->first_node("expr");
    if (expr != nullptr) {
      decl->init(Node<ExpressionNode>::dynPtrCast(parseXMLNode(expr)));
    }
    return decl;
  } else if (name == "branch") {
    auto branch = Node<BranchNode>::make();
    auto cond = node->first_node();
    if (cond == nullptr)
      throw XMLParseError("Missing condition in branch", {METADATA_PAIRS});
    branch->condition(Node<ExpressionNode>::dynPtrCast(parseXMLNode(cond)));
    auto success = cond->next_sibling();
    if (success == nullptr)
      throw XMLParseError("Missing success node in branch", {METADATA_PAIRS});
    branch->success(Node<BlockNode>::dynPtrCast(parseXMLNode(success)));
    auto blockFailiure = success->next_sibling("block");
    if (blockFailiure != nullptr) {
      branch->failiure<BlockNode>(Node<BlockNode>::dynPtrCast(parseXMLNode(blockFailiure)));
    }
    auto branchFailiure = success->next_sibling("branch");
    if (branchFailiure != nullptr) {
      branch->failiure<BranchNode>(Node<BranchNode>::dynPtrCast(parseXMLNode(branchFailiure)));
    }
    return branch;
  } else if (name == "loop") {
    auto loop = Node<LoopNode>::make();
    for (auto init = node->first_node("loop_init"); init; init = init->next_sibling("loop_init")) {
      loop->addInit(Node<DeclarationNode>::dynPtrCast(parseXMLNode(init)));
    }
    if (auto cond = node->first_node("loop_condition")) {
      loop->condition(Node<ExpressionNode>::dynPtrCast(parseXMLNode(cond)));
    }
    for (auto upd = node->first_node("loop_update"); upd; upd = upd->next_sibling("loop_update")) {
      loop->addUpdate(Node<ExpressionNode>::dynPtrCast(parseXMLNode(upd)));
    }
    if (auto code = node->first_node("block")) {
      loop->code(Node<BlockNode>::dynPtrCast(parseXMLNode(code)));
    }
    return loop;
  } else if (
      name == "loop_init" || name == "loop_condition" || name == "loop_update"
    ) {
    return parseXMLNode(node->first_node());
  } else if (name == "break") {
    return Node<BreakLoopNode>::make();
  } else if (name == "function") {
    std::string ident = safeAttr("ident");
    bool isForeign = boolAttr("foreign");
    auto n = Node<FunctionNode>::make(
      ident, FunctionSignature(funRet(), funArgs()), isForeign);
    funBlock(n);
    if (isForeign && ident.empty())
      throw XMLParseError("Can't have anonymous foreign function", {METADATA_PAIRS});
    return n;
  } else if (name == "type") {
    std::string typeName = requiredAttr("name");
    std::vector<std::string> inheritTypes = split(safeAttr("inherits"), ' ');
    auto typeNode = Node<TypeNode>::make(typeName, TypeList(ALL(inheritTypes)));
    parseChildren(node, typeNode);
    return typeNode;
  } else if (name == "member") {
    std::string ident = requiredAttr("ident");
    std::vector<std::string> types = split(safeAttr("types"), ' ');
    auto member = Node<MemberNode>::make(
      ident, TypeList(ALL(types)), boolAttr("static"), getVisibility());
    auto init = node->first_node("expr");
    if (init != nullptr) {
      member->init(Node<ExpressionNode>::staticPtrCast(parseXMLNode(init)));
    }
    return member;
  } else if (name == "method") {
    std::string ident = safeAttr("ident");
    bool isForeign = boolAttr("foreign");
    auto method = Node<MethodNode>::make(
      ident,
      FunctionSignature(funRet(), funArgs()),
      getVisibility(),
      boolAttr("static"),
      isForeign
    );
    funBlock(method);
    if (isForeign && ident.empty())
      throw XMLParseError("Can't have anonymous foreign method", {METADATA_PAIRS});
    return method;
  } else if (name == "constructor") {
    auto constructor = Node<ConstructorNode>::make(funArgs(), getVisibility());
    funBlock(constructor);
    return constructor;
  }
  throw XMLParseError("Unknown type of node", {METADATA_PAIRS, {"node name", name}});
}
