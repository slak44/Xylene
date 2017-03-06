#ifndef ERROR_HPP
#define ERROR_HPP

#include <stdexcept>
#include <string>
#include <sstream>
#include <map>
#include <termcolor/termcolor.hpp>
#include <fmt/format.h>

#include "util.hpp"
#include "trace.hpp"

#define METADATA_PAIRS {"file", __FILE__}, {"function", CURRENT_FUNCTION}, {"line", std::to_string(__LINE__)}

/**
  \brief Thrown when there is an issue in the user's code.
*/
class Error: public std::exception {
private:
  std::string message;
public:
  /**
    \brief Create an error. Consider using the string literals defined below.
    \param errType appears in front of the message
    \param msg error message
  */
  Error(std::string errType, std::string msg) noexcept;
    
  /// Format the existing message
  template <typename... Args>
  Error& operator()(Args&&... args) {
    message = fmt::format(message, std::forward<Args>(args)...);
    return *this;
  }

  /// Attach a Trace to the error message. Literally appends the Trace text
  Error& operator+(const Trace& rhs) noexcept {
    message += "\n\t" + rhs.toString();
    return *this;
  }
  
  const char* what() const noexcept override {
    return message.c_str();
  }
};

/// Define a new literal for Error types
#define LITERAL_MACRO(literalSuffix, messagePrefix) \
inline Error operator""_##literalSuffix(const char* msg, std::size_t) noexcept { \
  return Error(#messagePrefix, msg); \
}

LITERAL_MACRO(syntax, "Syntax error: ")
LITERAL_MACRO(badfloat, "Syntax error: Malformed float '{0}', ")
LITERAL_MACRO(type, "Type error: ")
LITERAL_MACRO(ref, "Reference error: ")

#undef LITERAL_MACRO

/**
  \brief Maps a description of data to its content
*/
using ErrorData = std::map<std::string, std::string>;

/**
  \brief Thrown when something goes wrong in this program.
*/
class InternalError: public std::runtime_error {
private:
  /// Internal function used to create the full error message
  static std::string buildErrorMessage(std::string errorName, std::string msg, const ErrorData& data);
protected:
  /**
    \brief Used by subclasses to replace InternalError in the error output.
  */
  InternalError(std::string errorName, std::string msg, const ErrorData& data);
public:
  /**
    \brief Construct an error.
    \param msg error message
    \param data debugging data about this error. Always add METADATA_PAIRS for file/function/line data.
  */
  InternalError(std::string msg, const ErrorData& data);
};

#endif
