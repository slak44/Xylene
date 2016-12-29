#include "utils/trace.hpp"

Range::Range(Position start, Position end): start(start), end(end) {}
Range::Range(Position where, uint charCount): start(where), end(Position(where.line, where.col + charCount)) {}

const Position Range::getStart() const noexcept {
  return start;
}
const Position Range::getEnd() const noexcept {
  return end;
}

std::string Range::toString() const noexcept {
  return "from " + start.toString() + " to " + end.toString();
}

Trace::Trace(std::string file, Range location): file(file), location(location) {}
Trace::Trace(std::nullptr_t): isNullTrace(true), location(Range(Position(0, 0), 0)) {}

const Range Trace::getRange() const noexcept {
  return location;
}

const std::string& Trace::getFileName() const noexcept {
  return file;
}

std::string Trace::toString() const noexcept {
  if (isNullTrace) return "";
  else return "found " + location.toString() + " in " + file;
}
