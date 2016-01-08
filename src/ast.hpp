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
      
      Type* booleanType = new Type("Boolean", {
        // Static members
      }, {
        // Instance members
      });
      
      Type* integerType = new Type("Integer", {
        // Static members
        {"MAX_VALUE", new Member(new Variable(new Integer(LLONG_MAX), {}), PUBLIC)},
        {"MIN_VALUE", new Member(new Variable(new Integer(LLONG_MIN), {}), PUBLIC)}
      }, {
        // Instance members
      });
      
      Type* floatType = new Type("Float", {
        // Static members
        {"MAX_VALUE", new Member(new Variable(new Float(FLT_MAX), {}), PUBLIC)},
        {"MIN_VALUE", new Member(new Variable(new Float(FLT_MIN), {}), PUBLIC)}
      }, {
        // Instance members
      });
      
      FunctionNode* stringLength = new FunctionNode("length", new Arguments {}, {});
      NativeBlockNode* stringLengthCode = new NativeBlockNode([=](ASTNode* funcScope) {
        Variable* context = resolveNameFrom(funcScope, "this");
        if (context->read() == nullptr) throw Error("'this' was not defined in this context", "NullPointerError", funcScope->getLineNumber());
        String* string = dynamic_cast<String*>(context->read());
        if (string == nullptr) throw Error("This function can only be called on Strings", "NullPointerError", funcScope->getLineNumber());
        return new Integer(string->getString().length());
      });
      stringLengthCode->setSelfInFunction(stringLength);
      
      Type* stringType = new Type("String", {
        // Static members
      }, {
        // Instance members
        {"length", new Member(new Variable(new Function(stringLength), {}), PUBLIC)}
      });
      
      Type* functionType = new Type("Function", {}, {});
      
      // Do not allow assignment by not specifying any allowed types for the Variable
      (*root.getScope())["Integer"] = new Variable(integerType, {});
      (*root.getScope())["Float"] = new Variable(floatType, {});
      (*root.getScope())["String"] = new Variable(stringType, {});
      (*root.getScope())["Boolean"] = new Variable(booleanType, {});
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
