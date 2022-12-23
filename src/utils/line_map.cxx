#include "line_map.hxx"

#include <cassert>
#include <algorithm>

void
PLineMap::add(uint32_t p_line_pos)
{
  m_positions.push_back(p_line_pos);
}

void
PLineMap::get_lineno_and_colno(uint32_t p_position, uint32_t* p_lineno, uint32_t* p_colno) const
{
  // Handle simple cases:
  if (m_positions.empty() || p_position < m_positions[0]) {
    if (p_lineno)
      *p_lineno = 1;
    if (p_colno)
      *p_colno = p_position + 1;
    return;
  } else if (p_position >= m_positions.back()) {
    if (p_lineno)
      *p_lineno = m_positions.size() + 1;
    if (p_colno)
      *p_colno = p_position - m_positions.back() + 1;
    return;
  }

  // General case (fallback to a binary search):
  const uint32_t upper_bound = search_rightmost(p_position);
  if (p_lineno)
    *p_lineno = upper_bound + 2;
  if (p_colno)
    *p_colno = p_position - m_positions[upper_bound] + 1;
}

uint32_t
PLineMap::get_lineno(uint32_t p_position) const
{
  uint32_t linepos;
  get_lineno_and_colno(p_position, &linepos, nullptr);
  return linepos;
}

uint32_t
PLineMap::get_colno(uint32_t p_position) const
{
  uint32_t colno;
  get_lineno_and_colno(p_position, nullptr, &colno);
  return colno;
}

uint32_t
PLineMap::get_line_start_pos(uint32_t p_lineno) const
{
  assert(p_lineno > 0 && p_lineno <= (m_positions.size() + 1));

  if (p_lineno == 1)
    return 0;
  else
    return m_positions[p_lineno - 2];
}

uint32_t
PLineMap::search_rightmost(uint32_t p_position) const
{
  uint32_t left = 0;
  uint32_t right = m_positions.size();

  while (left < right) {
    const uint32_t middle = (left + right) / 2;
    if (m_positions[middle] > p_position) {
      right = middle;
    } else {
      left = middle + 1;
    }
  }

  return right - 1;
}
