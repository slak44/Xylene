#include "utils/error.hpp"

Error::Error(std::string errType, std::string msg, Trace trace):
  runtime_error(errType + ": " + msg + "\n\t" + trace.toString()) {}

std::string InternalError::buildErrorMessage(std::string errorName, std::string msg, const ErrorData& data) {
  using namespace termcolor;
  std::stringstream ss;
  ss << red << errorName << reset << ": " << msg << "\n";
  for (auto& extra : data) {
    ss << "\t" << blue << extra.first << reset << ": " << extra.second << "\n";
  }
  return ss.str();
}

InternalError::InternalError(std::string errorName, std::string msg, const ErrorData& data):
  runtime_error(buildErrorMessage(errorName, msg, data)) {}

InternalError::InternalError(std::string msg, const ErrorData& data): InternalError("InternalError", msg, data) {}
