#ifndef BUILTINS_H_
#define BUILTINS_H_

#include <string>

namespace builtins {

class Object {
private:
public:
	Object();
	
	std::string asString();
};

class Integer : public Object {
private:
  long long int internal = 0;
public:
  Integer();
  Integer(std::string str, int base);
  Integer(long long int i);
  
  bool operator==(const Integer& right);
  bool operator!=(const Integer& right);
  bool operator< (const Integer& right);
  bool operator> (const Integer& right);
  bool operator>=(const Integer& right);
  bool operator<=(const Integer& right);
  
  Integer operator+(const Integer& right);
  Integer operator-(const Integer& right);
  Integer operator*(const Integer& right);
  Integer operator/(const Integer& right);
  Integer operator%(const Integer& right);
  
  Integer operator>>(const Integer& right);
  Integer operator<<(const Integer& right);
  
  Integer operator--();
  Integer operator++();
  Integer operator--(int);
  Integer operator++(int);
  
  Integer operator~();
  Integer operator&(const Integer& right);
  Integer operator^(const Integer& right);
  Integer operator|(const Integer& right);
};

}; /* namespace builtins */

#endif /* BUILTINS_H_ */
