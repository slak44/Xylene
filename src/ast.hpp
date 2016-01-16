#ifndef AST_HPP_
#define AST_HPP_

#include <string>

#include "global.hpp"
#include "builtins.hpp"
#include "nodes.hpp"
#include "functions.hpp"

namespace lang {
  class AbstractSyntaxTree {
  private:
    ASTNode root = ASTNode();
  public:
    AbstractSyntaxTree() {
      // TODO: add function to get args and do checks in nativecodeblocks
      FunctionNode* printNode = new FunctionNode("print", new Arguments {{"data", new Variable(nullptr, {"define"})}}, {});
      NativeBlockNode* bn = new NativeBlockNode([=](ASTNode* funcScope) {
        Variable* arg = resolveNameFrom(funcScope, "data");
        Object* somethingToPrint = arg->read();
        if (somethingToPrint == nullptr) return nullptr;
        auto type = somethingToPrint->getTypeData();
        while (type == "Variable") {
          somethingToPrint = dynamic_cast<Variable*>(somethingToPrint)->read();
          type = somethingToPrint->getTypeData();
        }
        if (type == "String") print(dynamic_cast<String*>(somethingToPrint)->getString());
        else if (type == "Integer") print(dynamic_cast<Integer*>(somethingToPrint)->getNumber());
        else if (type == "Float") print(dynamic_cast<Float*>(somethingToPrint)->getNumber());
        else if (type == "Boolean") print(dynamic_cast<Boolean*>(somethingToPrint)->value() ? "true" : "false");
        else if (type == "Function") print("Function " + dynamic_cast<Function*>(somethingToPrint)->getFNode()->getName());
        else if (type == "Type") print("Type " + dynamic_cast<Type*>(somethingToPrint)->getName());
        return nullptr;
      });
      bn->setSelfInFunction(printNode);
      (*root.getScope())["print"] = new Variable(new Function(printNode), {});
      
      Integer::integerType->getStaticMap().insert({"MAX_VALUE", new Member(new Variable(new Integer(LLONG_MAX), {}), PUBLIC)});
      Integer::integerType->getStaticMap().insert({"MIN_VALUE", new Member(new Variable(new Integer(LLONG_MIN), {}), PUBLIC)});
      
      Float::floatType->getStaticMap().insert({"MAX_VALUE", new Member(new Variable(new Float(FLT_MAX), {}), PUBLIC)});
      Float::floatType->getStaticMap().insert({"MIN_VALUE", new Member(new Variable(new Float(FLT_MIN), {}), PUBLIC)});
      
      FunctionNode* stringLength = new FunctionNode("length", new Arguments {}, {});
      NativeBlockNode* stringLengthCode = new NativeBlockNode([=](ASTNode* funcScope) {
        Variable* context = resolveNameFrom(funcScope, "this");
        while (context->read() != nullptr && context->read()->getTypeData() == "Variable") context = static_cast<Variable*>(context->read());
        if (context->read() == nullptr) throw Error("'this' was not defined in this context", "NullPointerError", funcScope->getLineNumber());
        String* string = dynamic_cast<String*>(context->read());
        if (string == nullptr) throw Error("This function can only be called on Strings", "NullPointerError", funcScope->getLineNumber());
        return new Integer(string->getString().length());
      });
      stringLengthCode->setSelfInFunction(stringLength);
      String::stringType->getInstanceMap().insert({"length", new Member(new Variable(new Function(stringLength), {}), PUBLIC)});
      
      FunctionNode* stringSubstr = new FunctionNode("substr", new Arguments {
        {"pos", new Variable(nullptr, {"Integer"})},
        {"len", new Variable(nullptr, {"Integer"})}
      }, {});
      NativeBlockNode* stringSubstrCode = new NativeBlockNode([=](ASTNode* funcScope) {
        Variable* context = resolveNameFrom(funcScope, "this");
        while (context->read() != nullptr && context->read()->getTypeData() == "Variable") context = static_cast<Variable*>(context->read());
        if (context->read() == nullptr) throw Error("'this' was not defined in this context", "NullPointerError", funcScope->getLineNumber());
        String* string = dynamic_cast<String*>(context->read());
        if (string == nullptr) throw Error("This function can only be called on Strings", "NullPointerError", funcScope->getLineNumber());
        auto nullErr = Error("One or more arguments are null", "NullPointerError", funcScope->getLineNumber());
        Variable* pos = resolveNameFrom(funcScope, "pos");
        Variable* len = resolveNameFrom(funcScope, "len");
        while (pos->read() != nullptr && pos->read()->getTypeData() == "Variable") pos = static_cast<Variable*>(pos->read());
        while (len->read() != nullptr && len->read()->getTypeData() == "Variable") len = static_cast<Variable*>(len->read());
        if (pos == nullptr || len == nullptr) throw nullErr;
        Integer* posInt = dynamic_cast<Integer*>(pos->read());
        Integer* lenInt = dynamic_cast<Integer*>(len->read());
        if (posInt == nullptr || lenInt == nullptr) throw nullErr;
        return new String(string->getString().substr(posInt->getNumber(), lenInt->getNumber()));
      });
      stringSubstrCode->setSelfInFunction(stringSubstr);
      String::stringType->getInstanceMap().insert({"substr", new Member(new Variable(new Function(stringSubstr), {}), PUBLIC)});
      
      Type* functionType = new Type("Function", {}, {});
      // Do not allow assignment by not specifying any allowed types for the Variable
      (*root.getScope())["Integer"] = new Variable(Integer::integerType, {});
      (*root.getScope())["Float"] = new Variable(Float::floatType, {});
      (*root.getScope())["String"] = new Variable(String::stringType, {});
      (*root.getScope())["Boolean"] = new Variable(Boolean::booleanType, {});
      (*root.getScope())["Function"] = new Variable(functionType, {});
    }
    
    void addRootChild(ASTNode* node) {
      root.addChild(node);
    }
    ChildrenNodes getRootChildren() {
      return root.getChildren();
    }
  };
  typedef AbstractSyntaxTree AST;
} /* namespace lang */

#endif /* AST_HPP_ */
