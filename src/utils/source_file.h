#pragma once

#include "line_map.h"

HEDLEY_BEGIN_C_DECLS

/*
 * A file that can be used as input for the lexer.
 *
 * Use p_source_file_open() to open a file and do not forget to
 * call p_source_file_close() when the file is no more used.
 */
typedef struct PSourceFile
{
  const char* filename;
  PLineMap line_map; // populated by the lexer
  const char* buffer; // the source file code, NUL-terminated
} PSourceFile;

PSourceFile*
p_source_file_open(const char* p_filename);

void
p_source_file_close(PSourceFile* p_file);

HEDLEY_END_C_DECLS
