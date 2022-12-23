#include "options.hxx"

POptions g_options = {
#define OPTION(p_opt, p_var) .p_var = false,
#define FEATURE_OPTION_SWITCH(p_opt, p_var, p_default) .p_var = (p_default),
#define FEATURE_OPTION_INT(p_opt, p_var, p_default) .p_var = (p_default),
#include "options.def"

  .output_file = nullptr,
};
