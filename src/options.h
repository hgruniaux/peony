#pragma once

#include "utils/dynamic_array.h"

#include <stdbool.h>

HEDLEY_BEGIN_C_DECLS

typedef enum POptimizationLevel
{
  P_OPT_O,
  P_OPT_O0,
  P_OPT_O1,
  P_OPT_O2,
  P_OPT_O3,
  P_OPT_Oz,
  P_OPT_Os
} POptimizationLevel;

typedef struct POptions
{
#define OPTION(p_opt, p_var) bool p_var;
#define FEATURE_OPTION_SWITCH(p_opt, p_var, p_default) bool p_var;
#define FEATURE_OPTION_INT(p_opt, p_var, p_default) int p_var;
#include "options.def"

  POptimizationLevel opt_optimization_level;

  const char* output_file;
  PDynamicArray input_files;
} POptions;

extern POptions g_options;

HEDLEY_END_C_DECLS
