#pragma once

#include <cstdint>
#include <vector>

/// PLineMap provides functions to convert between character positions and line
/// numbers for a compilation unit.
///
/// Character m_positions are a 0-based byte offset in the source file.
/// Line and column numbers are 1-based like many code editors for convenience.
///
/// The line map is populated by the lexer calling the PLineMap::add()
/// function. This one takes as a parameter the position of the beginning of a
/// new line with the constraint that the new position MUST BE greater than
/// the previous reported position (that way the internal line map m_positions
/// are guaranteed to be sorted).
class PLineMap
{
public:
  /// Adds a new line position (the position of the first byte of the newline, that is the position
  /// just after the character `\n` or `\r\n`).
  void add(uint32_t p_line_pos);

  /// Gets the line and column number corresponding to the given `p_pos` byte position.
  /// The parameters `p_lineno` or `p_colno` can be nullptr if the corresponding information is not needed.
  /// Moreover, both line and column numbers are 1-based.
  void get_lineno_and_colno(uint32_t p_pos, uint32_t* p_lineno, uint32_t* p_colno) const;
  /// Same as get_lineno_and_colno().
  [[nodiscard]] uint32_t get_lineno(uint32_t p_pos) const;
  /// Same as get_lineno_and_colno().
  [[nodiscard]] uint32_t get_colno(uint32_t p_pos) const;
  /// Gets the position of the first byte at the given line.
  [[nodiscard]] uint32_t get_line_start_pos(uint32_t p_lineno) const;

private:
  /// Does a binary search on the positions.
  [[nodiscard]] uint32_t search_rightmost(uint32_t p_pos) const;

  std::vector<uint32_t> m_positions;
};
