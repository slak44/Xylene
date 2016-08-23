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

Trace::Trace(Range location): location(location) {}

const Range Trace::getRange() const noexcept {
  return location;
}

std::string Trace::toString() const noexcept {
  return "found " + location.toString();
}
