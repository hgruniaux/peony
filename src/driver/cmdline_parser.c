#include "../options.h"
#include "../utils/diag.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void
print_help(const char* p_argv0)
{
  printf("Usage: %s [options] file...\n", p_argv0);
  printf("Options:\n");

  // TODO: implement help message
  printf("    ...\n");

  exit(1);
}

static bool
parse_feature_option_switch(const char* p_name, bool* p_var, const char* p_arg)
{
  bool value = true;
  if (memcmp(p_arg, "no-", 3) == 0) {
    p_arg += 3;
    value = false;
  }

  if (strcmp(p_name, p_arg) != 0)
    return false;

  *p_var = value;
  return true;
}

static bool
parse_feature_option_int(const char* p_name, const char* p_full_name, int* p_var, const char* p_arg)
{
  size_t name_len = strlen(p_name);
  if (memcmp(p_name, p_arg, name_len) != 0)
    return false;

  p_arg += name_len;
  if (*p_arg != '=')
    return false;
  p_arg += 1;

  if (*p_arg == '\0') {
    PDiag* d = diag(P_DK_err_missing_argument_cmdline_opt);
    diag_add_arg_str(d, p_full_name);
    diag_flush(d);
    return false;
  }

  int value = 0;
  while (*p_arg != '\0') {
    if (*p_arg >= '0' && *p_arg <= '9') {
      value *= 10;
      value += *p_arg - '0';
      continue;
    }

    PDiag* d = diag(P_DK_err_cmdline_opt_expect_int);
    diag_add_arg_str(d, p_full_name);
    diag_flush(d);
    return false;
  }

  *p_var = value;
  return true;
}

static bool
parse_optimization_level(const char* p_arg)
{
  POptimizationLevel level;
  if (strcmp(p_arg, "0") == 0)
    level = P_OPT_O0;
  else if (strcmp(p_arg, "1") == 0)
    level = P_OPT_O1;
  else if (strcmp(p_arg, "2") == 0)
    level = P_OPT_O2;
  else if (strcmp(p_arg, "3") == 0)
    level = P_OPT_O3;
  else if (strcmp(p_arg, "z") == 0)
    level = P_OPT_Oz;
  else if (strcmp(p_arg, "s") == 0)
    level = P_OPT_Os;
  else
    return false;

  g_options.opt_optimization_level = level;
  return true;
}

void
cmdline_parser(int p_argc, char* p_argv[])
{
  p_dynamic_array_init(&g_options.input_files);
  for (int i = 1; i < p_argc; ++i) {
    const char* arg = p_argv[i];
    if (*arg != '-') {
      p_dynamic_array_append(&g_options.input_files, (PDynamicArrayItem)arg);
      continue;
    }

    if (memcmp(arg, "-f", 2) == 0) {
      arg += 2;
#define FEATURE_OPTION_SWITCH(p_opt, p_var, p_default)                                                                 \
  if (parse_feature_option_switch((p_opt), &g_options.p_var, arg))                                                       \
    continue;
#define FEATURE_OPTION_INT(p_opt, p_var, p_default)                                                                    \
  if (parse_feature_option_int((p_opt), "-f" p_opt, &g_options.p_var, arg))                                              \
    continue;
#include "../options.def"

      arg -= 2;
    } else if (memcmp(arg, "-O", 2) == 0) {
      arg += 2;

      if (parse_optimization_level(arg))
        continue;

      arg -= 2;
    } else if (strcmp(arg, "--help") == 0) {
      print_help(p_argv[0]);
      return;
    } else if (strcmp(arg, "-o") == 0 || strcmp(arg, "--output") == 0) {
      if (i + 1 >= p_argc) {
        PDiag* d = diag(P_DK_err_missing_argument_cmdline_opt);
        diag_add_arg_str(d, arg);
        diag_flush(d);
        return;
      }

      i++;
      g_options.output_file = p_argv[i];
      continue;
    }

#if 0
#define OPTION(p_opt, p_var)                                                                                           \
  if (strcmp((p_opt), arg) == 0) {                                                                                       \
    g_options.p_var = true;                                                                                            \
    continue;                                                                                                          \
  }
#define FEATURE_OPTION(p_opt, p_var)
#include "../options.def"
#endif

    PDiag* d = diag(P_DK_err_unknown_cmdline_opt);
    diag_add_arg_str(d, arg);
    diag_flush(d);
  }

  if (g_options.input_files.size == 0) {
    PDiag* d = diag(P_DK_err_no_input_files);
    diag_flush(d);
    return;
  }

  if (g_options.output_file == NULL)
    g_options.output_file = "a.out";
}
