#include "source_file.hxx"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>

PSourceFile::PSourceFile(std::string p_filename, std::string p_content)
  : m_line_map()
  , m_filename(std::move(p_filename))
  , m_buffer(std::move(p_content))
{

}

std::unique_ptr<PSourceFile>
PSourceFile::open(std::string p_filename)
{
  FILE* stream = fopen(p_filename.c_str(), "rb");
  if (stream == nullptr)
    return nullptr;

  fseek(stream, 0, SEEK_END);
  const auto bufsize = (size_t)ftell(stream);
  fseek(stream, 0, SEEK_SET);

  std::string buffer;
  buffer.resize(bufsize);
  const size_t read_bytes = fread(buffer.data(), sizeof(char), bufsize, stream);
  fclose(stream);

  if (read_bytes != bufsize) {
    return nullptr;
  } else {
    return std::make_unique<PSourceFile>(std::move(p_filename), std::move(buffer));
  }
}
