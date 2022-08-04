#pragma once

#include "identifier_table.h"
#include "type.h"
#include "utils/string.h"

HEDLEY_BEGIN_C_DECLS

void
name_mangle(PIdentifierInfo* p_name, PFunctionType* p_func_type, PString* p_output);

HEDLEY_END_C_DECLS
