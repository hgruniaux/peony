#pragma once

#include "ast.h"

#include <llvm-c/Types.h>

struct PCodegenLLVM
{
  LLVMModuleRef module;
  LLVMBuilderRef builder;
  LLVMValueRef current_function;
  LLVMTypeRef current_function_type;
};

void
p_cg_init(struct PCodegenLLVM* p_cg);

void
p_cg_destroy(struct PCodegenLLVM* p_cg);

void
p_cg_dump(struct PCodegenLLVM* p_cg);

void
p_cg_optimize(struct PCodegenLLVM* p_cg, int p_opt_level);

void
p_cg_compile(struct PCodegenLLVM* p_cg, struct PAst* p_ast);
