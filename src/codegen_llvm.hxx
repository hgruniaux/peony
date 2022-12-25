#pragma once

#include "ast.hxx"
#include "context.hxx"

#include <llvm-c/Types.h>
#include <llvm-c/TargetMachine.h>

struct PCodegenLLVM
{
  PContext& context;
  LLVMModuleRef module;
  LLVMBuilderRef builder;
  LLVMValueRef current_function;
  LLVMTypeRef current_function_type;

  LLVMTargetMachineRef target_machine;

  int opt_level;

  PCodegenLLVM(PContext& p_context);
  ~PCodegenLLVM();
};

void
p_cg_dump(struct PCodegenLLVM* p_cg);

void
p_cg_compile(struct PCodegenLLVM* p_cg, PAst* p_ast, const char* p_output_filename);
