#include "../options.hxx"
#include "../utils/diag.hxx"

#include <hedley.h>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>

HEDLEY_NO_RETURN
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
  p_arg += 2; // skip '-f'
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
parse_feature_option_int(const char* p_name, int* p_var, const char* p_arg)
{
  p_arg += 2; // skip '-f'

  size_t name_len = strlen(p_name) - 2; // the two first characters are always '-f'
  if (memcmp(p_name + 2, p_arg, name_len) != 0)
    return false;

  p_arg += name_len;
  if (*p_arg == '\0' || (*p_arg == '=' && p_arg[1] == '\0')) {
    PDiag* d = diag(P_DK_err_missing_argument_cmdline_opt);
    diag_add_arg_str(d, p_name);
    diag_flush(d);
    return true; // there is no argument but this is the correct option so skip it
  }

  if (*p_arg != '=')
    return false;
  p_arg += 1;

  int value = 0;
  while (*p_arg != '\0') {
    if (*p_arg >= '0' && *p_arg <= '9') {
      value *= 10;
      value += *p_arg - '0';
      p_arg++;
      continue;
    }

    PDiag* d = diag(P_DK_err_cmdline_opt_expect_int);
    diag_add_arg_str(d, p_name);
    diag_flush(d);
    return true; // invalid argument but this is the correct option so skip it
  }

  *p_var = value;
  return true;
}

static bool
parse_optimization_level(const char* p_arg)
{
  assert(p_arg[0] == '-' && p_arg[1] == 'O');
  p_arg += 2;

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
  g_options.input_files.clear();
  for (int i = 1; i < p_argc; ++i) {
    const char* arg = p_argv[i];
    if (*arg != '-') {
      g_options.input_files.push_back(arg);
      continue;
    }

    if (memcmp(arg, "-f", 2) == 0) {
#define FEATURE_OPTION_SWITCH(p_opt, p_var, p_default)                                                                 \
  if (parse_feature_option_switch(p_opt, &g_options.p_var, arg))                                                  \
    continue;
#define FEATURE_OPTION_INT(p_opt, p_var, p_default)                                                                    \
  if (parse_feature_option_int("-f" p_opt, &g_options.p_var, arg))                                                     \
    continue;
#include "../options.def"
    } else if (memcmp(arg, "-O", 2) == 0 && parse_optimization_level(arg)) {
      continue;
    } else if (strcmp(arg, "--help") == 0) {
      print_help(p_argv[0]);
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

#define OPTION(p_opt, p_var)                                                                                           \
  if (strcmp(p_opt, arg) == 0) {                                                                                       \
    g_options.p_var = true;                                                                                            \
    continue;                                                                                                          \
  }
#define FEATURE_OPTION(p_opt, p_var)
#include "../options.def"

    PDiag* d = diag(P_DK_err_unknown_cmdline_opt);
    diag_add_arg_str(d, arg);
    diag_flush(d);
  }

  // Only emit "no input files" error if no error was already emitted.
  if (g_diag_context.diagnostic_count[P_DIAG_ERROR] == 0 && g_options.input_files.empty()) {
    PDiag* d = diag(P_DK_err_no_input_files);
    diag_flush(d);
    return;
  }

  if (g_options.output_file == nullptr)
    g_options.output_file = "a.out";
}
