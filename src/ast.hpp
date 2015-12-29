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
      // Do not allow assignment by not specifying any allowed types for the Variable
      (*root.getScope())[std::string("print")] = new Variable(new Function(printNode), {});
      (*root.getScope())[std::string("Integer")] = new Variable(integerType, {});
      (*root.getScope())[std::string("Float")] = new Variable(floatType, {});
      (*root.getScope())[std::string("String")] = new Variable(stringType, {});
      (*root.getScope())[std::string("Boolean")] = new Variable(booleanType, {});
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
