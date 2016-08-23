#ifndef TRACE_HPP
#define TRACE_HPP

#include <stack>

#include "utils/util.hpp"

class Range {
private:
  uint lineStart;
  uint lineEnd;
  uint colStart;
  uint colEnd;
public:
  Range(uint lineStart, uint lineEnd, uint colStart, uint colEnd);
  
  uint getLineStart() const noexcept;
  uint getLineEnd() const noexcept;
  uint getColStart() const noexcept;
  uint getColEnd() const noexcept;
};

class Trace {
private:
  Range location;
  // std::stack<?FunctionData?> stack; TODO
public:
  Trace(Range location);
  
  const Range& getRange() const noexcept;
};

#endif
