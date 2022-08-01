#include "source_location.h"

#include <assert.h>

void
p_source_location_get_lineno_and_colno(PSourceFile* p_source_file,
                                       PSourceLocation p_position,
                                       uint32_t* p_lineno,
                                       uint32_t* p_colno)
{
  assert(p_source_file != NULL);

  p_line_map_get_lineno_and_colno(&p_source_file->line_map, p_position, p_lineno, p_colno);
}
