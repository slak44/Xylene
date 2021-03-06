#ifndef UTIL_HPP
#define UTIL_HPP

#include <cstdint>
#include <iterator>
#include <string>
#include <sstream>
#include <iostream>
#include <vector>
#include <unordered_set>
#include <memory>
#include <algorithm>
#include <functional>
#include <experimental/filesystem>
#include <fmt/format.h>
#include <fmt/ostream.h>

namespace fs = std::experimental::filesystem;

#ifdef _MSC_VER
  #undef FILE_END
  #define __attribute__(attr)
  #define CURRENT_FUNCTION __FUNCSIG__
#else
  #define CURRENT_FUNCTION __PRETTY_FUNCTION__
#endif

#define UNUSED(expr) (void)(expr)
#define ALL(container) std::begin(container), std::end(container)
#define QUOTE_IMPLEMENTATION(x) #x
#define QUOTE(x) QUOTE_IMPLEMENTATION(x)

#ifdef __clang__
  #pragma clang diagnostic push
  // This only pulls one thing in the global namespace
  #pragma clang diagnostic ignored "-Wheader-hygiene"
#endif
  
  /// For ""s string literal
  using namespace std::literals::string_literals;

#ifdef __clang__
  #pragma clang diagnostic pop
#endif

/// Type for AbstractId's unique identifiers
using UniqueIdentifier = std::size_t;

/// Represents a list of types. Used in multiple places
using TypeList = std::unordered_set<std::string>;

/// Syntactic sugar
using TypeName = std::string;

/// Get a pretty address string from a pointer
std::string getAddressStringFrom(const void* ptr);

/**
  \brief Split a string into substrings
  \param delim where to break the string
*/
std::vector<std::string> split(const std::string& str, char delim);

/// Print something to stdout
template<typename T>
inline void print(T thing) {
  std::cout << thing;
}

/// Print a pointer address to stdout
template<typename T>
inline void print(T* thing) {
  std::cout << getAddressStringFrom(thing);
}

/// Print contents, not address for const char*
template<>
inline void print(const char* thing) {
  std::cout << thing;
}

/// Print something to stdout with a trailing newline
template<typename T>
inline void println(T thing) {
  print(thing);
  std::cout << std::endl;
}

/// Print multiple things (with spaces in-between) to stdout
template<typename T, typename... Args>
void print(T thing, Args... args) {
  print(thing);
  print(" ");
  print(args...);
}

/// Print multiple things (with spaces in-between) to stdout with a trailing newline
template<typename T, typename... Args>
void println(T thing, Args... args) {
  print(thing);
  print(" ");
  println(args...);
}

/// Checks if an item can be found in a container
template<typename Container>
bool includes(Container c, typename Container::value_type item) {
  return std::find(c.begin(), c.end(), item) != c.end();
}

/// Checks if an item can be found in a container
template<typename Container>
bool includes(Container c, std::function<bool(typename Container::value_type)> test) {
  return std::any_of(ALL(c), test);
}

/**
  \brief Binds a member function to an instance
  \param memberPtr the member function to be bound
  \param obj the instance to bind to
  \tparam ReturnType the member function's return type
  \tparam Args the member function's arguments' types
  \tparam Object the type of the instance
  
  Cannot handle cv-qualified stuff.
*/
template<typename ReturnType, typename... Args, typename Object>
std::function<ReturnType(Args...)> objBind(ReturnType(Object::*memberPtr)(Args...), Object* obj) {
  return [=](Args... args) -> ReturnType {return (obj->*memberPtr)(args...);};
}

/// Smart pointer utils
template<typename T>
struct PtrUtil {
  /// Shared pointer
  using Link = std::shared_ptr<T>;
  /// Weak pointer
  using WeakLink = std::weak_ptr<T>;
  
  /**
    \brief Use RTTI to determine if 2 pointers have the same type.
    \tparam U type stored in the shared ptr to compare to
    
    Compares this PtrUtil's template type with this function's template type.
  */
  template<typename U>
  static inline bool isSameType(const std::shared_ptr<U>& link) {
    return typeid(T) == typeid(*link);
  }
  
  /**
    \brief Wrapper around std::dynamic_pointer_cast.
    \tparam U stored in the shared ptr to be casted
  */
  template<typename U>
  static inline Link dynPtrCast(const std::shared_ptr<U>& link) {
    return std::dynamic_pointer_cast<T>(link);
  }
  
  /**
    \brief Wrapper around std::static_pointer_cast.
    \tparam U stored in the shared ptr to be casted
  */
  template<typename U>
  static inline Link staticPtrCast(const std::shared_ptr<U>& link) {
    return std::static_pointer_cast<T>(link);
  }
};

/// Identity function. Does literally nothing.
struct Identity {
  template<typename T>
  constexpr auto operator()(T&& v) const noexcept -> decltype(std::forward<T>(v)) {
    return std::forward<T>(v);
  }
};

static const auto defaultCombine __attribute((unused)) =
  [](std::string prev, std::string current) {
    return fmt::format("{0}, {1}", prev, current);
  };

/**
  \brief Collates a bunch of objects from Container into a string
  \tparam Container full type of source container (ex std::vector<int>)
  \param c the Container with objects
  \param objToString lambda to convert the object to a string (default is Identity)
  \param combine reduce-style lambda
*/
template<typename Container>
std::string collate(
  Container c,
  std::function<std::string(typename Container::value_type)> objToString = Identity(),
  std::function<std::string(std::string, std::string)> combine = defaultCombine
) {
  if (c.size() == 0) return "";
  if (c.size() == 1) return objToString(*c.begin());
  if (c.size() >= 2) {
    std::string str = objToString(*c.begin());
    std::for_each(std::next(std::begin(c)), std::end(c), [&](typename Container::value_type object) {
      str = combine(str, objToString(object));
    });
    return str;
  }
  throw std::logic_error("Size of container must be a positive integer");
}

#endif
