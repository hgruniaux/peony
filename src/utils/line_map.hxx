#pragma once

#include <hedley.h>

#include <cstdint>

HEDLEY_BEGIN_C_DECLS

/*
 * PLineMap provides functions to convert between character positions and line
 * numbers for a compilation unit.
 *
 * Character positions are a 0-based byte offset in the source file.
 * Line and column numbers are 1-based like many code editors for convenience.
 *
 * The line map is populated by the lexer calling the p_line_map_add()
 * function. This one takes as a parameter the position of the beginning of a
 * new line with the constraint that the new position MUST BE greater than
 * the previous reported position (that way the internal line map positions
 * are guaranteed to be sorted).
 */
typedef struct PLineMap
{
  uint32_t* positions;
  uint32_t capacity;
  uint32_t size;
} PLineMap;

void
p_line_map_init(PLineMap* p_lm);

void
p_line_map_destroy(PLineMap* p_lm);

void
p_line_map_add(PLineMap* p_lm, uint32_t p_line_pos);

void
p_line_map_get_lineno_and_colno(PLineMap* p_lm, uint32_t p_position, uint32_t* p_lineno, uint32_t* p_colno);

uint32_t
p_line_map_get_lineno(PLineMap* p_lm, uint32_t p_position);

uint32_t
p_line_map_get_colno(PLineMap* p_lm, uint32_t p_position);

uint32_t
p_line_map_get_line_start_position(PLineMap* p_lm, uint32_t p_lineno);

HEDLEY_END_C_DECLS
