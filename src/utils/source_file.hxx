#pragma once

#include "line_map.hxx"

/*
 * A file that can be used as input for the lexer.
 *
 * Use p_source_file_open() to open a file and do not forget to
 * call p_source_file_close() when the file is no more used.
 */
struct PSourceFile
{
  char* filename;
  PLineMap line_map; // populated by the lexer
  const char* buffer; // the source file code, NUL-terminated
};

PSourceFile*
p_source_file_open(const char* p_filename);

void
p_source_file_close(PSourceFile* p_file);
