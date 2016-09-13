#include "parser/xmlParser.hpp"

XMLParser::XMLParser() {}

XMLParser& XMLParser::parse(char* str) {
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
  tree = std::make_unique<AST>(AST(rootNode));
  return *this;
}

XMLParser& XMLParser::parse(rapidxml::file<char> xmlFile) {
  parse(xmlFile.data());
  return *this;
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
  auto safeAttr = [node](std::string what, std::string defaultValue = "") -> std::string {
    auto attr = node->first_attribute(what.c_str());
    if (attr != 0) return attr->value();
    else return defaultValue;
  };
  auto requiredAttr = [node](std::string what) -> std::string {
    auto attr = node->first_attribute(what.c_str());
    if (attr != 0) return attr->value();
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
      args.insert(std::make_pair(namePlusTypes[0], TypeList(ALL(types))));
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
      if (codeBlock == 0) throw XMLParseError("Method missing code block", {METADATA_PAIRS});
      f->setCode(Node<BlockNode>::staticPtrCast(parseXMLNode(codeBlock)));
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
    if (val != 0) retNode->setValue(Node<ExpressionNode>::dynPtrCast(parseXMLNode(val)));
    return retNode;
  } else if (name == "expr") {
    TokenType tokenType = getTokenTypeByName(requiredAttr("type"));
    std::string data = requiredAttr("value");
    std::unique_ptr<Token> content;
    if (tokenType == OPERATOR) {
      content = std::make_unique<Token>(tokenType, operatorIndexFrom(data), defaultTrace);
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
  } else if (name == "break") {
    return Node<BreakLoopNode>::make();
  } else if (name == "function") {
    std::string ident = safeAttr("ident");
    bool isForeign = boolAttr("foreign");
    auto n = Node<FunctionNode>::make(ident, FunctionSignature(funRet(), funArgs()), isForeign);
    funBlock(n);
    if (isForeign && ident.empty()) throw XMLParseError("Can't have anonymous foreign function", {METADATA_PAIRS});
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
    auto member = Node<MemberNode>::make(ident, TypeList(ALL(types)), boolAttr("static"), getVisibility());
    auto init = node->first_node("expr");
    if (init != 0) {
      member->setInit(Node<ExpressionNode>::staticPtrCast(parseXMLNode(init)));
    }
    return member;
  } else if (name == "method") {
    std::string ident = safeAttr("ident");
    bool isForeign = boolAttr("foreign");
    auto method = Node<MethodNode>::make(ident, FunctionSignature(funRet(), funArgs()), getVisibility(), boolAttr("static"), isForeign);
    funBlock(method);
    if (isForeign && ident.empty()) throw XMLParseError("Can't have anonymous foreign method", {METADATA_PAIRS});
    return method;
  } else if (name == "constructor") {
    auto constructor = Node<ConstructorNode>::make(funArgs(), getVisibility());
    funBlock(constructor);
    return constructor;
  }
  throw XMLParseError("Unknown type of node", {METADATA_PAIRS, {"node name", name}});
}

XMLParseError::XMLParseError(std::string msg, ErrorData data):
  InternalError("XMLParseError", msg, data) {}


