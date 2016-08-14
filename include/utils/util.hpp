#ifndef UTIL_HPP
#define UTIL_HPP

#include <string>
#include <sstream>
#include <iostream>
#include <vector>
#include <set>
#include <memory>
#include <functional>

#include "utils/suppressWarnings.hpp"

#define UNUSED(expr) (void)(expr)
#define ALL(container) std::begin(container), std::end(container)

using uint = unsigned int;
using int64 = long long;
using uint64 = unsigned long long;

using TypeList = std::set<std::string>;

// Print to stdout

template<typename T>
void println(T thing);

template<typename T>
void print(T thing);

template<typename T, typename... Args>
void println(T thing, Args... args);

template<typename T, typename... Args>
void print(T thing, Args... args);

// Vector utils

template<typename T>
bool contains(std::vector<T> vec, T item);

std::vector<std::string> split(const std::string& str, char delim);

template<typename T>
struct VectorHash;

// Pointer utils

template<typename T>
struct PtrUtil;

std::string getAddressStringFrom(const void* ptr);

// Collate into string

struct Identity;

static const auto defaultCollateCombine = [](std::string prev, std::string current) {return prev + ", " + current;};

template<typename Container>
std::string collate(Container c,
  std::function<std::string(const typename Container::value_type&)> objToString = Identity(),
  std::function<std::string(std::string, std::string)> combine = defaultCollateCombine
);

std::string collateTypeList(TypeList typeList);

#include "utils/util.tpp"

#endif
