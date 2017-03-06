#ifndef TRACE_HPP
#define TRACE_HPP

#include <stack>
#include <algorithm>
#include <fmt/format.h>

#include "utils/util.hpp"

/**
  \brief Tracks a position in a file.
*/
struct Position {
  uint64_t line;
  uint64_t col;
  
  inline constexpr Position(uint64_t line, uint64_t col) noexcept: line(line), col(col) {}
  
  inline std::string toString() const noexcept {
    return fmt::format("{0}:{1}", line, col);
  }
};

inline std::ostream& operator<<(std::ostream& os, const Position& pos) noexcept {
  return os << pos.toString();
}

/**
  \brief Tracks a range from a position to another in a file.
*/
class Range {
private:
  Position start;
  Position end;
public:
  inline constexpr Range(Position start, Position end) noexcept: start(start), end(end) {}
  inline constexpr Range(Position where, unsigned charCount) noexcept: start(where), end(Position(where.line, where.col + charCount)) {}
  
  inline constexpr Position getStart() const noexcept {
    return start;
  }
  inline constexpr Position getEnd() const noexcept {
    return end;
  }
  
  inline std::string toString() const noexcept {
    return fmt::format("from {0} to {1}", start, end);
  }
};

inline std::ostream& operator<<(std::ostream& os, const Range& r) noexcept {
  return os << r.toString();
}

/**
  \brief Stores debug information.
*/
class Trace {
private:
  bool isNullTrace = false;
  std::string file;
  Range location;
  // std::stack<?FunctionData?> stack; TODO
public:
  inline Trace(std::string file, Range location) noexcept: file(file), location(location) {}
  inline Trace(std::nullptr_t) noexcept: isNullTrace(true), location(Range(Position(0, 0), 0)) {}
  
  inline Range getRange() const noexcept {
    return location;
  }
  inline std::string getFileName() const noexcept {
    return file;
  }
  
  inline std::string toString() const noexcept {
    if (isNullTrace) return "";
    else return fmt::format("found {0} in {1}", location, file);
  }
};

inline std::ostream& operator<<(std::ostream& os, const Trace& tr) noexcept {
  return os << tr.toString();
}

/// Default trace object that has an empty range
const Trace defaultTrace = Trace("<none>", Range(Position(0, 0), Position(0, 0)));

#endif
