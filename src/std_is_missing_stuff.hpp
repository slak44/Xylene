// Stuff the standard lib should have
#ifndef STD_IS_MISSING_STUFF_H_
#define STD_IS_MISSING_STUFF_H_

#include <string>
#include <vector>
#include <iostream>
#include <algorithm>
#include <sstream>

template<class T>
std::ostream& operator<<(std::ostream& os, std::vector<T>& vec) {
  for (unsigned int i = 0; i < vec.size(); ++i) os << vec[i] << std::endl;
  return os;
}

template <typename T>
bool contains(T element, std::vector<T> vec) {
  return std::find(vec.begin(), vec.end(), element) != vec.end();
}

#endif /* STD_IS_MISSING_STUFF_H_ */
