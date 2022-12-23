#include "source_file.hxx"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>

PSourceFile*
p_source_file_open(const char* p_filename)
{
  assert(p_filename != nullptr);

  FILE* file = fopen(p_filename, "rb");
  if (file == nullptr)
    return nullptr;

  fseek(file, 0, SEEK_END);
  const auto bufsize = (size_t)ftell(file);
  fseek(file, 0, SEEK_SET);

  auto* source_file = new PSourceFile;
  assert(source_file != nullptr);

  const size_t filename_len = strlen(p_filename) + 1 /* NUL-terminated */;
  source_file->filename = static_cast<char*>(malloc(sizeof(char) * filename_len));
  assert(source_file->filename != nullptr);
  memcpy(source_file->filename, p_filename, sizeof(char) * filename_len);

  char* buffer = static_cast<char*>(malloc(sizeof(char) * (bufsize + 1)));
  const size_t read_bytes = fread(buffer, sizeof(char), bufsize, file);
  buffer[bufsize] = '\0';

  fclose(file);

  if (read_bytes != bufsize) {
    free(source_file->filename);
    free(buffer);
    free(source_file);
    return nullptr;
  }

  source_file->buffer = buffer;
  return source_file;
}

void
p_source_file_close(PSourceFile* p_file)
{
  if (p_file == nullptr)
    return;

  free((void*)p_file->buffer);
  free(p_file->filename);
  delete p_file;
}
