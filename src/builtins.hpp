#ifndef BUILTINS_H_
#define BUILTINS_H_

#include <string>
#include <map>
#include <functional>
#include <stdexcept>
#include "global.hpp"
#include "operators.hpp"

namespace builtins {

std::map<ops::Operator*, void*> initializeOperatorMap(std::vector<void*> funcs);

class Object {
public:
  /*
   * 'void*' points to a std::function.
   * The template arguments for it depend on the type of Object.
   * For example, Integer will have a binary Operator "+", whose function signature can look like this: std::function<Integer*(Integer, Integer)>
   */
  static std::map<ops::Operator*, void*> operators;

  Object();
	virtual ~Object();
	
	virtual std::string asString();
//	std::map<ops::Operator, void*> getOperations();
	virtual std::string getTypeData();
};

// TODO: maybe Number base class?

class Float : public Object {
private:
  double64 internal = 0.0;
public:
  static std::map<ops::Operator*, void*> operators;
  
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
  static std::map<ops::Operator*, void*> operators;
  
  Integer();
  Integer(std::string str, int base);
  Integer(int64 i);
  
  std::string asString();
  std::string getTypeData();
  
//  bool operator==(const Integer& right);
//  bool operator!=(const Integer& right);
//  bool operator< (const Integer& right);
//  bool operator> (const Integer& right);
//  bool operator>=(const Integer& right);
//  bool operator<=(const Integer& right);
//  
//  Integer operator+(const Integer& right);
//  Integer operator-(const Integer& right);
//  Integer operator*(const Integer& right);
//  Integer operator/(const Integer& right);
//  Integer operator%(const Integer& right);
//  
//  Integer operator>>(const Integer& right);
//  Integer operator<<(const Integer& right);
//  
//  Integer operator--();
//  Integer operator++();
//  Integer operator--(int);
//  Integer operator++(int);
//  
//  Integer operator~();
//  Integer operator&(const Integer& right);
//  Integer operator^(const Integer& right);
//  Integer operator|(const Integer& right);
};

}; /* namespace builtins */

#endif /* BUILTINS_H_ */
