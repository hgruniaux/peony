#include "source_file.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

PSourceFile*
p_source_file_open(const char* p_filename)
{
  assert(p_filename != NULL);

  FILE* file = fopen(p_filename, "rb");
  if (file == NULL)
    return NULL;

  fseek(file, 0, SEEK_END);
  long bufsize = ftell(file);
  fseek(file, 0, SEEK_SET);

  PSourceFile* source_file = malloc(sizeof(PSourceFile));
  assert(source_file != NULL);

  size_t filename_len = strlen(p_filename) + 1 /* NUL-terminated */;
  source_file->filename = malloc(sizeof(char) * filename_len);
  assert(source_file->filename != NULL);
  memcpy(source_file->filename, p_filename, sizeof(char) * filename_len);

  char* buffer = malloc(sizeof(char) * (bufsize + 1));
  const size_t read_bytes = fread(buffer, sizeof(char), bufsize, file);
  buffer[bufsize] = '\0';

  fclose(file);

  if (read_bytes != bufsize) {
    free(source_file->filename);
    free(buffer);
    free(source_file);
    return NULL;
  }

  source_file->buffer = buffer;
  p_line_map_init(&source_file->line_map);
  return source_file;
}

void
p_source_file_close(PSourceFile* p_file)
{
  if (p_file == NULL)
    return;

  p_line_map_destroy(&p_file->line_map);
  free(p_file->buffer);
  free(p_file->filename);
  free(p_file);
}
