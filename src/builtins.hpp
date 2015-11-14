#ifndef BUILTINS_H_
#define BUILTINS_H_

#include <string>
#include <unordered_map>
#include <functional>
#include <stdexcept>

#include "global.hpp"
#include "operators.hpp"

namespace builtins {

/*
 * 'void*' points to a std::function.
 * The template arguments for it depend on the type of Object.
 * For example, Integer will have a binary Operator "+", whose function signature can look like this: std::function<Integer*(Integer*, Integer*)>
 * There should be typedefs for common std::functions such as BinaryOp in Integer.
 */
typedef std::unordered_map<ops::Operator, void*, ops::OperatorHash> OperatorMap;

OperatorMap initializeOperatorMap(std::vector<void*> funcs);

class Object {
public:
  static OperatorMap operators;

  Object();
	virtual ~Object();
	
	virtual std::string asString();
	virtual std::string getTypeData();
};

// TODO: maybe Number base class?

class Float : public Object {
private:
  double64 internal = 0.0;
public:
  static OperatorMap operators;
  
  Float();
  Float(std::string str);
  Float(double64 f);
  
  std::string asString();
  std::string getTypeData();
};

class Integer : public Object {
private:
  int64 internal = 0;
public:
  typedef std::function<builtins::Integer*(builtins::Integer*, builtins::Integer*)> BinaryOp;
  static OperatorMap operators;
  
  Integer();
  Integer(std::string str, int base);
  Integer(int64 i);
  
  std::string asString();
  std::string getTypeData();
};

}; /* namespace builtins */

#endif /* BUILTINS_H_ */
