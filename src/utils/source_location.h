#pragma once

#include "source_file.h"

HEDLEY_BEGIN_C_DECLS

/*
 * A source location is really just a byte offset into a source file.
 */
typedef uint32_t PSourceLocation;

/*
 * A closed source range. It is just a pair of source locations which
 * represents the begin and end of the range (both inclusives).
 */
typedef struct PSourceRange
{
  PSourceLocation begin;
  PSourceLocation end;
} PSourceRange;

/* Wrapper around p_line_map_get_lineno_and_colno().
 * Lines and columns are 1-numbered. */
void
p_source_location_get_lineno_and_colno(PSourceFile* p_source_file,
                                       PSourceLocation p_position,
                                       uint32_t* p_lineno,
                                       uint32_t* p_colno);


HEDLEY_END_C_DECLS
