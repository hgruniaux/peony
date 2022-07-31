#pragma once

#include <stdbool.h>

typedef enum PDiagSeverity
{
  P_DIAG_UNSPECIFIED,
  P_DIAG_NOTE,
  P_DIAG_WARNING,
  P_DIAG_ERROR,
  P_DIAG_FATAL
} PDiagSeverity;

extern const char* CURRENT_FILENAME;
extern unsigned int CURRENT_LINENO;
extern bool g_verify_mode_enabled;

void
note(const char* p_msg, ...);

void
warning(const char* p_msg, ...);

void
error(const char* p_msg, ...);

/* Verify mode interface (used for testsuite): */

void
verify_expect_warning(const char* p_msg_begin, const char* p_msg_end);

void
verify_expect_error(const char* p_msg_begin, const char* p_msg_end);

void
verify_finalize(void);
