#pragma once

#include "ast.hxx"

#include <llvm-c/Types.h>
#include <llvm-c/TargetMachine.h>

struct PCodegenLLVM
{
  LLVMModuleRef module;
  LLVMBuilderRef builder;
  LLVMValueRef current_function;
  LLVMTypeRef current_function_type;

  LLVMTargetMachineRef target_machine;

  int opt_level;
};

void
p_cg_init(struct PCodegenLLVM* p_cg);

void
p_cg_destroy(struct PCodegenLLVM* p_cg);

void
p_cg_dump(struct PCodegenLLVM* p_cg);

void
p_cg_compile(struct PCodegenLLVM* p_cg, PAst* p_ast, const char* p_output_filename);
