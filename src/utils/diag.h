#pragma once

typedef enum PDiagSeverity
{
  P_DIAG_NOTE,
  P_DIAG_WARN,
  P_DIAG_ERROR,
  P_DIAG_FATAL
} PDiagSeverity;

extern const char* CURRENT_FILENAME;
extern unsigned int CURRENT_LINENO;

void
note(const char* p_msg, ...);

void
warning(const char* p_msg, ...);

void
error(const char* p_msg, ...);
