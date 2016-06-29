#ifndef ERROR_HPP
#define ERROR_HPP

#include <stdexcept>
#include <string>
#include <sstream>
#include <map>

#include "util.hpp"

#define GET_ERR_METADATA __FILE__, __PRETTY_FUNCTION__, __LINE__
#define ERR_METADATA_TYPES(arg1, arg2, arg3) std::string arg1, std::string arg2, int arg3
#define METADATA_PAIRS_FROM(file, func, line) {"file", file}, {"function", func}, {"line", std::to_string(line)}
#define METADATA_PAIRS {"file", __FILE__}, {"function", __PRETTY_FUNCTION__}, {"line", std::to_string(__LINE__)}

class Error: public std::runtime_error {
public:
  Error(std::string errType, std::string msg, uint64 line):
    runtime_error(errType + ": " + msg + ", at line " + std::to_string(line)) {}
};

typedef std::map<std::string, std::string> ErrorData;

class InternalError: public std::runtime_error {
private:
  static std::string buildErrorMessage(std::string msg, ErrorData data) {
    std::stringstream ss;
    ss << "\n" << "InternalError: " << msg << "\n";
    for (auto& extra : data) {
      ss << "\t" << extra.first << ": " << extra.second << "\n";
    }
    return ss.str();
  }
public:
  InternalError(std::string msg, ErrorData data):
    runtime_error(buildErrorMessage(msg, data)) {}
};

#endif
