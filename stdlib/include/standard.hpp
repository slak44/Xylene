#ifndef STANDARD_HPP
#define STANDARD_HPP

#include <unordered_map>
#include <string>

#include "io.hpp"

/**
  \brief Maps standard library function names to pointers to those functions.
*/
const std::unordered_map<std::string, void*> nameToFunPtr {
  {"printC", reinterpret_cast<void*>(printC)}
};

#endif
