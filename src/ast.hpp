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
      FunctionNode* printNode = new FunctionNode(std::string("print"), new Arguments {{"data", new Variable(nullptr, {"define"})}}, {});
      NativeBlockNode* bn = new NativeBlockNode([=](ASTNode* funcScope) {
        Variable* arg = resolveNameFrom(funcScope, "data");
        Object* somethingToPrint = arg->read();
        if (somethingToPrint == nullptr) return;
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
      });
      bn->setSelfInFunction(printNode);
      (*root.getScope())[std::string("print")] = new Variable(new Function(printNode), {});
      // TODO: find a non-retarded way to define members (pass the instance to the member calls for example)
      Type* integerType = new Type(std::string("Integer"), {
        // Static members
        {"MAX_VALUE", new Member(new Variable(new Integer(LLONG_MAX), {}), PUBLIC)},
        {"MIN_VALUE", new Member(new Variable(new Integer(LLONG_MIN), {}), PUBLIC)}
      }, {
        // Instance members
      });
      (*root.getScope())[std::string("Integer")] = new Variable(integerType, {}); // Do not allow assignment by not specifying any allowed types for the Variable
      Type* floatType = new Type(std::string("Float"), {
        // Static members
        {"MAX_VALUE", new Member(new Variable(new Float(FLT_MAX), {}), PUBLIC)},
        {"MIN_VALUE", new Member(new Variable(new Float(FLT_MIN), {}), PUBLIC)}
      }, {
        // Instance members
      });
      (*root.getScope())[std::string("Float")] = new Variable(floatType, {});
      Type* stringType = new Type(std::string("String"), {
        // Static members
      }, {
        // Instance members
      });
      (*root.getScope())[std::string("String")] = new Variable(stringType, {});
      Type* booleanType = new Type(std::string("Boolean"), {
        // Static members
      }, {
        // Instance members
        // TODO: implement .length and .substr() so rule 110 can work
      });
      (*root.getScope())[std::string("Boolean")] = new Variable(booleanType, {});
      Type* functionType = new Type(std::string("Function"), {
        // Static members
      }, {
        // Instance members
      });
      (*root.getScope())[std::string("Function")] = new Variable(functionType, {});
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
