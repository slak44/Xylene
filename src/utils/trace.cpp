#include "utils/trace.hpp"

Range::Range(uint lineStart, uint lineEnd, uint colStart, uint colEnd):
  lineStart(lineStart),
  lineEnd(lineEnd),
  colStart(colStart),
  colEnd(colEnd) {}
  
uint Range::getLineStart() const noexcept {
  return lineStart;
}
uint Range::getLineEnd() const noexcept {
  return lineEnd;
}
uint Range::getColStart() const noexcept {
  return colStart;
}
uint Range::getColEnd() const noexcept {
  return colEnd;
}

Trace::Trace(Range location): location(location) {}

const Range& Trace::getRange() const noexcept {
  return location;
}
