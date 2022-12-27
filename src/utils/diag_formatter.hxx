#pragma once

#include "diag.hxx"

#include <string>

void
p_diag_format_arg(std::string& p_buffer, PDiagArgument* p_arg);

void
p_diag_format_msg(std::string& p_buffer, const char* p_msg, PDiagArgument* p_args, size_t p_arg_count);

/// Prints the given set of source ranges along the source lines to stderr.
void
p_diag_print_source_ranges(PSourceFile* p_file, PSourceRange* p_ranges, size_t p_range_count);

/// Prints a source line from the given file to stderr in the form:
///     4 | fn foo() -> i32 {
///
/// Returns the line length in bytes.
size_t
p_diag_print_source_line(PSourceFile* p_file, uint32_t p_lineno);
