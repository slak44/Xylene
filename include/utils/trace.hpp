#ifndef TRACE_HPP
#define TRACE_HPP

#include <stack>

#include "utils/util.hpp"

/**
  \brief Tracks a position in a file.
*/
struct Position {
  uint64 line;
  uint64 col;
  
  inline Position(uint64 line, uint64 col): line(line), col(col) {}
  
  inline std::string toString() const noexcept {
    return std::to_string(line) + ":" + std::to_string(col);
  }
};

/**
  \brief Tracks a range from a position to another in a file.
*/
class Range {
private:
  Position start;
  Position end;
public:
  Range(Position start, Position end);
  Range(Position where, uint charCount);
  
  const Position getStart() const noexcept;
  const Position getEnd() const noexcept;
  
  std::string toString() const noexcept;
};

/**
  \brief Stores debug information.
*/
class Trace {
private:
  Range location;
  // std::stack<?FunctionData?> stack; TODO
public:
  Trace(Range location);
  
  const Range getRange() const noexcept;
  
  std::string toString() const noexcept;
};

/// Default trace object that has an empty range
const Trace defaultTrace = Trace(Range(Position(0, 0), Position(0, 0)));

#endif
