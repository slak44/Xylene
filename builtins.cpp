#include "builtins.h"

namespace builtins {


Integer::Integer() {
  
};

Integer::Integer(std::string str, int base) {
  this->internal = std::stoll(str, 0, base);
}

Integer::Integer(long long int i): internal(i) {
    
}

bool Integer::operator==(const Integer& right) {return this->internal == right.internal;}
bool Integer::operator!=(const Integer& right) {return !operator==(right);}
bool Integer::operator< (const Integer& right) {return this->internal < right.internal;}
bool Integer::operator> (const Integer& right) {return this->internal > right.internal;}
bool Integer::operator>=(const Integer& right) {return !operator<(right);}
bool Integer::operator<=(const Integer& right) {return !operator>(right);}

Integer Integer::operator+(const Integer& right) {return this->internal + right.internal;};
Integer Integer::operator-(const Integer& right) {return this->internal - right.internal;};
Integer Integer::operator*(const Integer& right) {return this->internal * right.internal;};
Integer Integer::operator/(const Integer& right) {return this->internal / right.internal;};
Integer Integer::operator%(const Integer& right) {return this->internal % right.internal;};

Integer Integer::operator>>(const Integer& right) {return this->internal >> right.internal;};
Integer Integer::operator<<(const Integer& right) {return this->internal << right.internal;};

Integer Integer::operator--()    {return --(this->internal);};
Integer Integer::operator++()    {return ++(this->internal);};
Integer Integer::operator--(int) {return (this->internal)--;};
Integer Integer::operator++(int) {return (this->internal)++;};

Integer Integer::operator~() {return ~(this->internal);};

Integer Integer::operator&(const Integer& right) {return this->internal & right.internal;};
Integer Integer::operator^(const Integer& right) {return this->internal ^ right.internal;};
Integer Integer::operator|(const Integer& right) {return this->internal | right.internal;};

};
