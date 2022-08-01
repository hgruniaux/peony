#include "line_map.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_CAPACITY 16
#define GROWTH_CAPACITY(p_capacity) ((p_capacity)*2)

void
p_line_map_init(PLineMap* p_lm)
{
  assert(p_lm != NULL);

  p_lm->size = 0;
  p_lm->capacity = INITIAL_CAPACITY;
  p_lm->positions = malloc(sizeof(uint32_t) * INITIAL_CAPACITY);
  assert(p_lm->positions != NULL);
}

void
p_line_map_destroy(PLineMap* p_lm)
{
  assert(p_lm != NULL);

  p_lm->size = 0;
  p_lm->capacity = 0;
  free(p_lm->positions);
  p_lm->positions = NULL;
}

void
p_line_map_add(PLineMap* p_lm, uint32_t p_line_pos)
{
  assert(p_lm != NULL);
  assert(p_lm->size == 0 || p_line_pos > p_lm->positions[p_lm->size - 1]);

  if (p_lm->size == p_lm->capacity) {
    p_lm->capacity = GROWTH_CAPACITY(p_lm->capacity);
    p_lm->positions = realloc(p_lm->positions, sizeof(uint32_t) * p_lm->capacity);
    assert(p_lm->positions != NULL);
  }

  p_lm->positions[p_lm->size++] = p_line_pos;
}

static uint32_t
line_map_search_rightmost(PLineMap* p_lm, uint32_t p_position)
{
  assert(p_lm->size > 0);

  uint32_t left = 0;
  uint32_t right = p_lm->size;

  while (left < right) {
    const uint32_t middle = (left + right) / 2;
    if (p_lm->positions[middle] > p_position) {
      right = middle;
    } else {
      left = middle + 1;
    }
  }

  return right - 1;
}

void
p_line_map_get_lineno_and_colno(PLineMap* p_lm, uint32_t p_position, uint32_t* p_lineno, uint32_t* p_colno)
{
  assert(p_lm != NULL && p_lm->positions != NULL);

  // Handle simple cases:
  if (p_lm->size == 0 || p_position < p_lm->positions[0]) {
    if (p_lineno)
      *p_lineno = 1;
    if (p_colno)
      *p_colno = p_position + 1;
    return;
  } else if (p_position >= p_lm->positions[p_lm->size - 1]) {
    if (p_lineno)
      *p_lineno = p_lm->size + 1;
    if (p_colno)
      *p_colno = p_position - p_lm->positions[p_lm->size - 1] + 1;
    return;
  }

  // General case (fallback to a binary search):
  const uint32_t upper_bound = line_map_search_rightmost(p_lm, p_position);
  if (p_lineno)
    *p_lineno = upper_bound + 2;
  if (p_colno)
    *p_colno = p_position - p_lm->positions[upper_bound] + 1;
}

uint32_t
p_line_map_get_lineno(PLineMap* p_lm, uint32_t p_position)
{
  assert(p_lm != NULL && p_lm->positions != NULL);

  uint32_t linepos;
  p_line_map_get_lineno_and_colno(p_lm, p_position, &linepos, NULL);
  return linepos;
}

uint32_t
p_line_map_get_colno(PLineMap* p_lm, uint32_t p_position)
{
  assert(p_lm != NULL && p_lm->positions != NULL);

  uint32_t colno;
  p_line_map_get_lineno_and_colno(p_lm, p_position, NULL, &colno);
  return colno;
}

uint32_t
p_line_map_get_line_start_position(PLineMap* p_lm, uint32_t p_lineno)
{
  assert(p_lm != NULL && p_lm->positions != NULL);
  assert(p_lineno > 0 && p_lineno <= (p_lm->size + 1));

  if (p_lineno == 1)
    return 0;
  else
    return p_lm->positions[p_lineno - 2];
}
