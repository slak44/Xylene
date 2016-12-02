#ifndef ERROR_HPP
#define ERROR_HPP

#include <stdexcept>
#include <string>
#include <sstream>
#include <map>
#include <termcolor/termcolor.hpp>

#include "util.hpp"
#include "trace.hpp"

#define METADATA_PAIRS {"file", __FILE__}, {"function", CURRENT_FUNCTION}, {"line", std::to_string(__LINE__)}

/**
  \brief Thrown when there is an issue in the user's code.
*/
class Error: public std::runtime_error {
public:
  /**
    \brief Create an error.
    \param errType appears in front of the message
    \param msg error message
    \param trace where in the user's code this happened
  */
  Error(std::string errType, std::string msg, Trace trace);
};

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
