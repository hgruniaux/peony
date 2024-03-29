#pragma once

#include "line_map.hxx"

#include <string>
#include <memory>

/// A file that can be used as input for the lexer.
struct PSourceFile
{
  PSourceFile(std::string p_path, std::string p_content);

  static std::unique_ptr<PSourceFile> open(std::string p_filename);

  [[nodiscard]] const char* get_buffer_raw() const { return m_buffer.data(); }
  [[nodiscard]] const std::string& get_buffer() const { return m_buffer; }
  [[nodiscard]] const std::string& get_filename() const { return m_filename; }
  [[nodiscard]] const std::string& get_path() const { return m_path; }

  [[nodiscard]] PLineMap& get_line_map() { return m_line_map; }
  [[nodiscard]] const PLineMap& get_line_map() const { return m_line_map; }

private:
  // Populated by the lexer
  PLineMap m_line_map;
  std::string m_filename;
  std::string m_path;
  std::string m_buffer;
};
