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

ASTNode::Link XMLParser::parseXMLNode(rapidxml::xml_node<>* node) {
  if (node == nullptr) throw XMLParseError("Null node", {METADATA_PAIRS});
  std::string name = node->name();
  if (name == "block") {
    auto typeAttr = node->first_attribute("type");
    std::string type = typeAttr == 0 ? "code" : typeAttr->value();
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
    TokenType tokenType = getTokenTypeByName(node->first_attribute("type")->value());
    auto val = node->first_attribute("value");
    if (val == 0) throw XMLParseError("Can't find expression value attribute", {METADATA_PAIRS});
    std::string data = val->value();
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
  } else if (name == "break") {
    return Node<BreakLoopNode>::make();
  } else if (name == "function") {
    std::string ident;
    auto identAttr = node->first_attribute("ident");
    if (identAttr != 0) ident = identAttr->value();
    else ident = "";
    std::vector<std::string> returnTypes {};
    auto returnAttr = node->first_attribute("return");
    if (returnAttr != 0) returnTypes = split(returnAttr->value(), ' ');
    FunctionSignature::Arguments args {};
    auto argsAttr = node->first_attribute("args");
    if (argsAttr != 0) {
      std::vector<std::string> stringArgs = split(argsAttr->value(), ',');
      for (auto& arg : stringArgs) {
        std::vector<std::string> namePlusTypes = split(arg, ':');
        std::vector<std::string> types = split(namePlusTypes[1], ' ');
        args.insert(std::make_pair(namePlusTypes[0], TypeList(ALL(types))));
      }
    }
    auto codeBlock = node->first_node("block");
    if (codeBlock == 0) throw XMLParseError("Function missing code block", {METADATA_PAIRS});
    auto n = Node<FunctionNode>::make(ident, FunctionSignature(returnAttr == 0 ? nullptr : TypeInfo(TypeList(ALL(returnTypes))), args));
    n->setCode(Node<BlockNode>::staticPtrCast(parseXMLNode(codeBlock)));
    return n;
  }
  throw XMLParseError("Unknown type of node", {METADATA_PAIRS, {"node name", name}});
}

XMLParseError::XMLParseError(std::string msg, ErrorData data):
  InternalError("XMLParseError", msg, data) {}


