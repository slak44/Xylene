#ifndef UTIL_HPP
#define UTIL_HPP

#include <string>
#include <sstream>
#include <iostream>
#include <vector>
#include <set>
#include <memory>
#include <typeinfo>
#include <functional>

#include "suppressWarnings.hpp"

#define UNUSED(expr) (void)(expr)
#define ALL(container) std::begin(container), std::end(container)

using uint = unsigned int;
using int64 = long long;
using uint64 = unsigned long long;

using TypeList = std::set<std::string>;

template<typename T>
void println(T thing) {
  std::cout << thing << std::endl;
}

template<typename T>
void print(T thing) {
  std::cout << thing;
}

template<typename T, typename... Args>
void println(T thing, Args... args) {
  if (sizeof...(args) == 1) {
    print(thing);
    print(" ");
    print(args...);
    std::cout << std::endl;
    return;
  }
  print(thing);
  print(" ");
  print(args...);
}

template<typename T, typename... Args>
void print(T thing, Args... args) {
  print(thing);
  print(" ");
  print(args...);
}

template<typename T>
bool contains(std::vector<T> vec, T item) {
  return std::find(vec.begin(), vec.end(), item) != vec.end();
}

std::vector<std::string> split(const std::string& str, char delim) {
  std::vector<std::string> vec {};
  std::stringstream ss(str);
  std::string item;
  while (getline(ss, item, delim)) vec.push_back(item);
  return vec;
}

template<typename T>
struct PtrUtil {
  using U = std::unique_ptr<T>;
  using Link = std::shared_ptr<T>;
  using WeakLink = std::weak_ptr<T>;
  
  template<typename... Args>
  static U unique(Args... args) {
    return std::make_unique<T>(args...);
  }
  
  template<typename... Args>
  static Link make(Args... args) {
    return std::make_shared<T>(args...);
  }
  
  template<typename U>
  static inline bool isSameType(std::shared_ptr<U> link) {
    return typeid(T) == typeid(*link);
  }
  
  template<typename U>
  static inline Link dynPtrCast(std::shared_ptr<U> link) {
    return std::dynamic_pointer_cast<T>(link);
  }
};

struct Identity {
  template<typename T>
  constexpr auto operator()(T&& v) const noexcept -> decltype(std::forward<T>(v)) {
    return std::forward<T>(v);
  }
};

template<typename T>
struct VectorHash {
  std::size_t operator()(const std::vector<T>& vec) const {
    std::size_t seed = vec.size();
    std::hash<T> hash;
    for (const T& i : vec) {
      seed ^= hash(i) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    }
    return seed;
  }
};

std::string getAddressStringFrom(const void* ptr) {
  std::stringstream ss;
  ss << "0x" << std::hex << reinterpret_cast<std::uintptr_t>(ptr);
  return ss.str();
}

#define COLLATE_TYPE const typename Container::value_type&

static const auto defaultCollateCombine = [](std::string prev, std::string current) {return prev + ", " + current;};

template<typename Container>
std::string collate(Container c,
  std::function<std::string(COLLATE_TYPE)> objToString = Identity(),
  std::function<std::string(std::string, std::string)> combine = defaultCollateCombine
  ) {
  if (c.size() == 0) return "";
  if (c.size() == 1) return objToString(*c.begin());
  if (c.size() >= 2) {
    std::string str = objToString(*c.begin());
    std::for_each(++c.begin(), c.end(), [&str, &objToString, &combine](COLLATE_TYPE object) {
      str = combine(str, objToString(object));
    });
    return str;
  }
  throw std::logic_error("Size of container must be a positive integer");
}

#undef COLLATE_TYPE

std::string collateTypeList(TypeList typeList) {
  return collate<TypeList>(typeList);
}

#endif
