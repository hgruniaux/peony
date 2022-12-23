#pragma once

#include "identifier_table.hxx"
#include "type.hxx"

#include <string>

HEDLEY_BEGIN_C_DECLS

void
name_mangle(PIdentifierInfo* p_name, PFunctionType* p_func_type, std::string& p_output);

HEDLEY_END_C_DECLS
