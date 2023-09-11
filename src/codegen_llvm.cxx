#include "codegen_llvm.hxx"

#include <llvm/Analysis/CGSCCPassManager.h>
#include <llvm/Analysis/LoopAnalysisManager.h>
#include <llvm/IR/DIBuilder.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Verifier.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>

#include <stack>
#include <string>

static llvm::StringRef
to_str_ref(PIdentifierInfo* p_name)
{
  auto spelling = p_name->get_spelling();
  return { spelling.data(), spelling.size() };
}

struct PCodeGenLLVM::D
{
  PContext& ctx;

  llvm::TargetMachine* target_machine;
  std::unique_ptr<llvm::LLVMContext> llvm_ctx;
  std::unique_ptr<llvm::Module> llvm_module;
  std::unique_ptr<llvm::IRBuilder<>> builder;
  std::unique_ptr<llvm::DIBuilder> debug_builder;

  std::unordered_map<PType*, llvm::Type*> types_cache;
  std::unordered_map<PType*, llvm::DIType*> debug_types_cache;
  std::unordered_map<const PDecl*, llvm::Value*> decls;

  llvm::DICompileUnit* debug_compile_unit;
  llvm::DIFile* debug_file;
  PSourceFile* current_file;
  std::stack<llvm::DIScope*> lexical_blocks;

  struct LoopInfoEntry
  {
    // Entry block to use to implement the break statement.
    llvm::BasicBlock* break_bb;
    // Entry block to use to implement the continue statement.
    llvm::BasicBlock* continue_bb;
  };

  std::stack<LoopInfoEntry> loop_infos;

  D(PContext& p_ctx)
    : ctx(p_ctx)
  {
    llvm_ctx = std::make_unique<llvm::LLVMContext>();
    llvm_module = std::make_unique<llvm::Module>("", *llvm_ctx);
    builder = std::make_unique<llvm::IRBuilder<>>(*llvm_ctx);
    debug_builder = std::make_unique<llvm::DIBuilder>(*llvm_module);
  }

  [[nodiscard]] const llvm::DataLayout& get_data_layout() { return llvm_module->getDataLayout(); }

  llvm::DIType* to_debug_func_ty_impl(PFunctionType* p_func_ty)
  {
    std::vector<llvm::Metadata*> param_types(p_func_ty->get_param_count() + 1);
    param_types[0] = to_debug_ty(p_func_ty->get_ret_ty());
    for (size_t i = 0; i < p_func_ty->get_param_count(); ++i) {
      param_types[i + 1] = to_debug_ty(p_func_ty->get_params()[i]);
    }

    return debug_builder->createSubroutineType(debug_builder->getOrCreateTypeArray(param_types));
  }

  llvm::DIType* to_debug_struct_ty_impl(PStructDecl* p_struct_decl)
  {
    auto fields = p_struct_decl->get_fields();
    std::vector<llvm::Metadata*> elements(fields.size());
    for (size_t i = 0; i < fields.size(); ++i)
      elements[i] = to_debug_ty(fields[i]->get_type());

    llvm::Type* llvm_type = to_llvm_ty(p_struct_decl->get_type());
    auto align_in_bits = get_data_layout().getPrefTypeAlign(llvm_type).value();
    auto size_in_bits = get_data_layout().getTypeSizeInBits(llvm_type).getFixedSize();
    return debug_builder->createStructType(nullptr,
                                           to_str_ref(p_struct_decl->get_name()),
                                           debug_file,
                                           0,
                                           size_in_bits,
                                           align_in_bits,
                                           llvm::DINode::DIFlags::FlagZero,
                                           nullptr,
                                           debug_builder->getOrCreateArray(elements));
  }

  llvm::DIType* to_debug_ty_impl(PType* p_type)
  {
    using namespace llvm::dwarf;
    switch (p_type->get_kind()) {
      case P_TK_VOID:
        return nullptr;
      case P_TK_BOOL:
        return debug_builder->createBasicType("bool", 1, DW_ATE_boolean);
      case P_TK_CHAR:
        return debug_builder->createBasicType("char", 32, DW_ATE_unsigned_char);
      case P_TK_I8:
        return debug_builder->createBasicType("i8", 8, DW_ATE_signed);
      case P_TK_I16:
        return debug_builder->createBasicType("i16", 16, DW_ATE_signed);
      case P_TK_I32:
        return debug_builder->createBasicType("i32", 32, DW_ATE_signed);
      case P_TK_I64:
        return debug_builder->createBasicType("i64", 64, DW_ATE_signed);
      case P_TK_U8:
        return debug_builder->createBasicType("u8", 8, DW_ATE_unsigned);
      case P_TK_U16:
        return debug_builder->createBasicType("u16", 16, DW_ATE_unsigned);
      case P_TK_U32:
        return debug_builder->createBasicType("u32", 32, DW_ATE_unsigned);
      case P_TK_U64:
        return debug_builder->createBasicType("u64", 64, DW_ATE_unsigned);
      case P_TK_F32:
        return debug_builder->createBasicType("f32", 32, DW_ATE_float);
      case P_TK_F64:
        return debug_builder->createBasicType("f64", 64, DW_ATE_float);

      case P_TK_PAREN:
        return to_debug_ty(p_type->as<PParenType>()->get_sub_type());

      case P_TK_FUNCTION:
        return to_debug_func_ty_impl(p_type->as<PFunctionType>());

      case P_TK_POINTER: {
        auto* pointee_ty = to_debug_ty(p_type->as<PPointerType>()->get_element_ty());
        return debug_builder->createPointerType(pointee_ty, target_machine->getPointerSizeInBits(0));
      }

      case P_TK_ARRAY:
        assert(false && "array types not yet implemented");
        return nullptr;

      case P_TK_TAG: {
        auto* decl = p_type->as<PTagType>()->get_decl();
        switch (decl->get_kind()) {
          case P_DK_STRUCT:
            return to_debug_struct_ty_impl(decl->as<PStructDecl>());
          default:
            assert(false && "unreachable");
            return nullptr;
        }
      }

      case P_TK_UNKNOWN:
        assert(false && "unknown types should not reach codegen");
        return nullptr;

      default:
        assert(false && "unreachable");
        return nullptr;
    }
  }

  /// Converts a Peony type to a LLVM debug DWARF type.
  /// This function can return nullptr.
  llvm::DIType* to_debug_ty(PType* p_type)
  {
    assert(p_type != nullptr);

    // Unlike to_llvm_ty() we do not use canonical type for lookup because
    // for debugging we really want the type as written by the user.

    const auto it = debug_types_cache.find(p_type);
    if (it != debug_types_cache.end())
      return it->second;

    auto* llvm_type = to_debug_ty_impl(p_type);
    debug_types_cache.insert({ p_type, llvm_type });
    return llvm_type;
  }

  llvm::Type* to_llvm_func_ty_impl(PFunctionType* p_func_type)
  {
    assert(p_func_type->is_canonical_ty());

    auto* ret_ty = to_llvm_ty(p_func_type->get_ret_ty());

    std::vector<llvm::Type*> param_types(p_func_type->get_param_count());
    for (size_t i = 0; i < p_func_type->get_param_count(); ++i) {
      param_types[i] = to_llvm_ty(p_func_type->get_params()[i]);
    }

    return llvm::FunctionType::get(ret_ty, param_types, false);
  }

  llvm::Type* to_llvm_struct_ty_impl(PStructDecl* p_struct_decl)
  {
    auto fields = p_struct_decl->get_fields();
    std::vector<llvm::Type*> field_types(fields.size());
    for (size_t i = 0; i < fields.size(); ++i) {
      field_types[i] = to_llvm_ty(fields[i]->get_type());
    }

    if (p_struct_decl->get_name() != nullptr) {
      return llvm::StructType::create(*llvm_ctx, field_types, to_str_ref(p_struct_decl->get_name()));
    } else {
      return llvm::StructType::create(*llvm_ctx, field_types);
    }
  }

  llvm::Type* to_llvm_ty_impl(PType* p_can_type)
  {
    switch (p_can_type->get_kind()) {
      case P_TK_VOID:
        return llvm::Type::getVoidTy(*llvm_ctx);
      case P_TK_CHAR:
        return llvm::Type::getInt32Ty(*llvm_ctx);
      case P_TK_BOOL:
        return llvm::Type::getInt1Ty(*llvm_ctx);
      case P_TK_I8:
      case P_TK_U8:
        return llvm::Type::getInt8Ty(*llvm_ctx);
      case P_TK_I16:
      case P_TK_U16:
        return llvm::Type::getInt16Ty(*llvm_ctx);
      case P_TK_I32:
      case P_TK_U32:
        return llvm::Type::getInt32Ty(*llvm_ctx);
      case P_TK_I64:
      case P_TK_U64:
        return llvm::Type::getInt64Ty(*llvm_ctx);
      case P_TK_F32:
        return llvm::Type::getFloatTy(*llvm_ctx);
      case P_TK_F64:
        return llvm::Type::getDoubleTy(*llvm_ctx);

      case P_TK_PAREN:
        assert(false && "parenthesized types are never canonical types");
        return nullptr;

      case P_TK_FUNCTION:
        return to_llvm_func_ty_impl(p_can_type->as<PFunctionType>());

      case P_TK_POINTER:
        // Now LLVM pointers are opaque.
        return llvm::PointerType::get(*llvm_ctx, 0);

      case P_TK_ARRAY: {
        auto* array_ty = p_can_type->as<PArrayType>();
        auto* elt_ty = to_llvm_ty(array_ty->get_element_ty());
        return llvm::ArrayType::get(elt_ty, array_ty->get_num_elements());
      }

      case P_TK_TAG: {
        auto* decl = p_can_type->as<PTagType>()->get_decl();
        switch (decl->get_kind()) {
          case P_DK_STRUCT:
            return to_llvm_struct_ty_impl(decl->as<PStructDecl>());
          default:
            assert(false && "unreachable");
            return nullptr;
        }
      }

      case P_TK_UNKNOWN:
        assert(false && "unknown types should not reach codegen");
        return nullptr;

      default:
        assert(false && "unimplemented type");
        return nullptr;
    }
  }

  /// Converts a Peony type to a LLVM type.
  llvm::Type* to_llvm_ty(PType* p_type)
  {
    assert(p_type != nullptr);

    p_type = p_type->get_canonical_ty();

    const auto it = types_cache.find(p_type);
    if (it != types_cache.end())
      return it->second;

    auto* llvm_type = to_llvm_ty_impl(p_type);
    assert(llvm_type != nullptr);
    types_cache.insert({ p_type, llvm_type });
    return llvm_type;
  }

  /// Gets the current function where the IR builder is located.
  llvm::Function* get_function() { return builder->GetInsertBlock()->getParent(); }

  /// Inserts a `alloca` instruction in the entry block of the current function.
  /// LLVM prefers that alloca instruction are inserted in the entry block
  /// because in that case they are guaranteed to be executed only once and
  /// makes mem2reg pass simpler.
  /// This function returns the inserted llvm::AllocaInst*.
  llvm::Value* insert_alloc_in_entry_bb(llvm::Type* p_type)
  {
    auto* func = get_function();
    llvm::IRBuilder<> b(&func->getEntryBlock(), func->getEntryBlock().begin());
    return b.CreateAlloca(p_type);
  }

  /// Inserts a dummy block just after the current insertion block
  /// and position the builder at end of it. Used to ensure that
  /// each terminal instruction is at end of a block (when generating
  /// code for `break;` or `return;` statements for example).
  void insert_dummy_block()
  {
    auto* func = get_function();
    auto* bb = llvm::BasicBlock::Create(*llvm_ctx, "", func);
    builder->SetInsertPoint(bb);
  }

  void reset_location() { builder->SetCurrentDebugLocation(llvm::DebugLoc()); }
  void emit_location(PSourceLocation p_src_loc)
  {
    if (current_file == nullptr)
      return;

    auto* scope = get_current_di_scope();

    uint32_t lineno, colno;
    p_source_location_get_lineno_and_colno(current_file, p_src_loc, &lineno, &colno);
    builder->SetCurrentDebugLocation(llvm::DILocation::get(*llvm_ctx, lineno, colno, scope));
  }

  llvm::DIScope* get_current_di_scope()
  {
    if (lexical_blocks.empty())
      return debug_compile_unit;
    else
      return lexical_blocks.top();
  }
  llvm::DILocation* get_di_location(PSourceLocation p_src_loc)
  {
    uint32_t lineno, colno;
    p_source_location_get_lineno_and_colno(current_file, p_src_loc, &lineno, &colno);
    return llvm::DILocation::get(*llvm_ctx, lineno, colno, get_current_di_scope());
  }

  llvm::DILocalVariable* emit_var_info(const PVarDecl* p_decl)
  {
    return debug_builder->createAutoVariable(
      get_current_di_scope(), to_str_ref(p_decl->get_name()), debug_file, 0, to_debug_ty(p_decl->get_type()));
  }

  void finish_func_codegen()
  {
    auto* func = get_function();

    // Fix degenerate blocks (without a terminator) that were generated.
    bool returns_void = func->getReturnType()->isVoidTy();
    for (auto& block : func->getBasicBlockList()) {
      if (block.getTerminator() != nullptr)
        continue;

      llvm::IRBuilder<> b(&block);
      if (returns_void) {
        b.CreateRetVoid();
      } else {
        b.CreateUnreachable();
      }
    }

    // Our code generation and the above fix for degenerate blocks tends to
    // generate many unreachable blocks. Remove them now. This is not strictly
    // necessary as optimization passes will do it, but yet we do it nevertheless
    // for unoptimized builds and debugging dump clarity.
    llvm::EliminateUnreachableBlocks(*func);
  }
};

PCodeGenLLVM::PCodeGenLLVM(PContext& p_ctx)
  : m_d(new D(p_ctx))
{
}

PCodeGenLLVM::~PCodeGenLLVM() = default;

bool
PCodeGenLLVM::codegen(PAstTranslationUnit* p_ast)
{
  m_d->current_file = p_ast->p_src_file;
  m_d->debug_file = m_d->debug_builder->createFile(p_ast->p_src_file->get_filename(), ".");
  m_d->debug_compile_unit =
    m_d->debug_builder->createCompileUnit(llvm::dwarf::DW_LANG_C, m_d->debug_file, "Peony Compiler", true, "", 0);

  auto target_triple = llvm::sys::getDefaultTargetTriple();
  m_d->llvm_module->setTargetTriple(target_triple);

  std::string error;
  auto target = llvm::TargetRegistry::lookupTarget(target_triple, error);

  // Print an error and exit if we couldn't find the requested target.
  // This generally occurs if we've forgotten to initialise the
  // TargetRegistry or we have a bogus target triple.
  if (!target) {
    llvm::errs() << error;
    return false;
  }

  auto cpu = "generic";
  auto features = "";

  llvm::TargetOptions opt;
  auto rm = llvm::Optional<llvm::Reloc::Model>();
  m_d->target_machine = target->createTargetMachine(target_triple, cpu, features, opt, rm);

  m_d->llvm_module->setDataLayout(m_d->target_machine->createDataLayout());
  m_d->llvm_module->setTargetTriple(target_triple);

  visit(p_ast);
  m_d->llvm_module->dump();
  m_d->debug_builder->finalize();
  assert(!llvm::verifyModule(*m_d->llvm_module, &llvm::errs()));
  return true;
}

void
PCodeGenLLVM::optimize()
{
  // Create the analysis managers.
  llvm::LoopAnalysisManager lam;
  llvm::FunctionAnalysisManager fam;
  llvm::CGSCCAnalysisManager cgam;
  llvm::ModuleAnalysisManager mam;

  // Create the new pass manager builder.
  // Take a look at the PassBuilder constructor parameters for more
  // customization, e.g. specifying a TargetMachine or various debugging
  // options.
  llvm::PassBuilder pb;

  // Register all the basic analyses with the managers.
  pb.registerModuleAnalyses(mam);
  pb.registerCGSCCAnalyses(cgam);
  pb.registerFunctionAnalyses(fam);
  pb.registerLoopAnalyses(lam);
  pb.crossRegisterProxies(lam, fam, cgam, mam);

  // Create the pass manager.
  // This one corresponds to a typical -O2 optimization pipeline.
  llvm::ModulePassManager mpm = pb.buildPerModuleDefaultPipeline(llvm::OptimizationLevel::O2);

  // Optimize the IR!
  mpm.run(*m_d->llvm_module, mam);
}

bool
PCodeGenLLVM::write_llvm_ir(const std::string& p_filename)
{
  std::error_code ec;
  llvm::raw_fd_stream stream(p_filename, ec);
  assert(!ec);
  m_d->llvm_module->print(stream, nullptr);
  return true;
}

bool
PCodeGenLLVM::write_object_file(const std::string& p_filename)
{
  std::error_code ec;
  llvm::raw_fd_ostream stream(p_filename, ec);
  if (ec) {
    llvm::errs() << "Could not open file: " << ec.message();
    return false;
  }

  llvm::legacy::PassManager pass;
  auto file_type = llvm::CGFT_ObjectFile;

  if (m_d->target_machine->addPassesToEmitFile(pass, stream, nullptr, file_type)) {
    llvm::errs() << "TargetMachine can't emit a file of this type";
    return false;
  }

  pass.run(*m_d->llvm_module);
  return true;
}

void*
PCodeGenLLVM::visit_translation_unit(const PAstTranslationUnit* p_node)
{
  m_d->emit_location(p_node->get_source_range().begin);
  visit(p_node->decls);
  return nullptr;
}

void*
PCodeGenLLVM::visit_compound_stmt(const PAstCompoundStmt* p_node)
{
  uint32_t lineno, colno;
  p_source_location_get_lineno_and_colno(m_d->current_file, p_node->get_source_range().begin, &lineno, &colno);
  m_d->lexical_blocks.push(
    m_d->debug_builder->createLexicalBlock(m_d->get_current_di_scope(), m_d->debug_file, lineno, colno));

  m_d->emit_location(p_node->get_source_range().begin);
  visit(p_node->stmts);

  m_d->lexical_blocks.pop();
  return nullptr;
}

void*
PCodeGenLLVM::visit_let_stmt(const PAstLetStmt* p_node)
{
  m_d->emit_location(p_node->get_source_range().begin);
  visit(p_node->var_decls);
  return nullptr;
}

void*
PCodeGenLLVM::visit_break_stmt(const PAstBreakStmt* p_node)
{
  assert(!m_d->loop_infos.empty());

  m_d->emit_location(p_node->get_source_range().begin);
  auto* break_bb = m_d->loop_infos.top().break_bb;
  assert(break_bb != nullptr);
  m_d->builder->CreateBr(break_bb);
  m_d->insert_dummy_block();
  return nullptr;
}

void*
PCodeGenLLVM::visit_continue_stmt(const PAstContinueStmt* p_node)
{
  assert(!m_d->loop_infos.empty());

  m_d->emit_location(p_node->get_source_range().begin);
  auto* continue_bb = m_d->loop_infos.top().break_bb;
  assert(continue_bb != nullptr);
  m_d->builder->CreateBr(continue_bb);
  m_d->insert_dummy_block();
  return nullptr;
}

void*
PCodeGenLLVM::visit_return_stmt(const PAstReturnStmt* p_node)
{
  m_d->emit_location(p_node->get_source_range().begin);

  if (p_node->ret_expr == nullptr) {
    m_d->builder->CreateRetVoid();
    return nullptr;
  }

  auto* ret_value = static_cast<llvm::Value*>(visit(p_node->ret_expr));
  if (p_node->ret_expr->get_type(m_d->ctx)->is_void_ty())
    m_d->builder->CreateRetVoid();

  m_d->builder->CreateRet(ret_value);
  m_d->insert_dummy_block();
  return nullptr;
}

void*
PCodeGenLLVM::visit_loop_stmt(const PAstLoopStmt* p_node)
{
  // The generated code:
  //  1 | body_bb:
  //  2 |     ; body...
  //  3 |     br body_bb
  //  4 | exit_bb: ; used to implement the break instruction
  //  5 |     ; ...

  m_d->emit_location(p_node->get_source_range().begin);
  auto* func = m_d->get_function();

  auto* body_bb = llvm::BasicBlock::Create(*m_d->llvm_ctx, "", func);
  auto* exit_bb = llvm::BasicBlock::Create(*m_d->llvm_ctx, "", func);
  m_d->loop_infos.push({ exit_bb, body_bb });

  m_d->builder->CreateBr(body_bb);

  // Body block:
  m_d->builder->SetInsertPoint(body_bb);
  visit(p_node->body_stmt);
  m_d->builder->CreateBr(body_bb);

  // Continue block:
  m_d->builder->SetInsertPoint(exit_bb);

  m_d->loop_infos.pop();
  return nullptr;
}

void*
PCodeGenLLVM::visit_while_stmt(const PAstWhileStmt* p_node)
{
  // The generated code:
  //  1 | entry_bb:
  //  2 |     ; cond...
  //  3 |     br ..., body_bb, continue_bb
  //  4 | body_bb:
  //  5 |     ; body...
  //  5 |     br entry_bb
  //  6 | exit_bb: ; used to implement the break instruction
  //  7 |     ; ...
  //
  // Which corresponds to the Peony equivalent code:
  //  1 | loop {
  //  2 |     if (!cond) { break; }
  //  3 |
  //  4 |     % body...
  //  5 | }

  m_d->emit_location(p_node->get_source_range().begin);
  auto* func = m_d->get_function();

  auto* entry_bb = llvm::BasicBlock::Create(*m_d->llvm_ctx, "", func);
  auto* body_bb = llvm::BasicBlock::Create(*m_d->llvm_ctx, "", func);
  auto* exit_bb = llvm::BasicBlock::Create(*m_d->llvm_ctx, "", func);
  m_d->loop_infos.push({ exit_bb, entry_bb });

  m_d->builder->CreateBr(entry_bb);

  // Entry block:
  m_d->builder->SetInsertPoint(entry_bb);
  auto* condition = static_cast<llvm::Value*>(visit(p_node->cond_expr));
  m_d->builder->CreateCondBr(condition, body_bb, exit_bb);

  // Body block:
  m_d->builder->SetInsertPoint(body_bb);
  visit(p_node->body_stmt);
  m_d->builder->CreateBr(entry_bb);

  // Continue block:
  m_d->builder->SetInsertPoint(exit_bb);

  m_d->loop_infos.pop();
  return nullptr;
}

/// Returns the inner logical not expression if there is one, otherwise returns nullptr.
/// This function do not check if p_expr is really a expression with the bool type.
static const PAstUnaryExpr*
get_logical_not_if_any(const PAstExpr* p_expr)
{
  p_expr = p_expr->ignore_parens_and_casts();
  if (p_expr->get_kind() == P_SK_UNARY_EXPR && p_expr->as<PAstUnaryExpr>()->opcode == P_UNARY_NOT) {
    return p_expr->as<PAstUnaryExpr>();
  } else {
    return nullptr;
  }
}

void*
PCodeGenLLVM::visit_if_stmt(const PAstIfStmt* p_node)
{
  // The generated code:
  //  1 |     br ..., body_bb, continue_bb
  //  2 | then_bb:
  //  3 |     ; body...
  //  4 |     br continue_bb
  //  5 | else_bb:
  //  6 |     ; else stmt...
  //  7 |     br continue_bb
  //  8 | continue_bb:
  //  9 |     ; ...

  m_d->emit_location(p_node->get_source_range().begin);

  // OPTIMIZATION: If the condition expression is a constant expression only
  // codegen the required branch.
  std::optional<bool> cond_expr_value = p_node->cond_expr->eval_as_bool(m_d->ctx);
  if (cond_expr_value.has_value()) {
    if (cond_expr_value.value()) {
      // if (true) { then_stmt } else { else_stmt } <=> then_stmt
      visit(p_node->then_stmt);
    } else if (p_node->else_stmt != nullptr) { // && !cond_expr_value.value()
      // if (false) { then_stmt } else { else_stmt } <=> else_stmt
      visit(p_node->else_stmt);
    }

    return nullptr;
  }

  auto* func = m_d->get_function();

  auto* then_bb = llvm::BasicBlock::Create(*m_d->llvm_ctx, "", func);
  auto* continue_bb = llvm::BasicBlock::Create(*m_d->llvm_ctx, "", func);
  llvm::BasicBlock* else_bb = nullptr;
  if (p_node->else_stmt != nullptr)
    else_bb = llvm::BasicBlock::Create(*m_d->llvm_ctx, "", func);

  // OPTIMIZATION: If the condition expression is a logical not, i.e. of the form `!sub_expr`,
  // and there is a `else` branch, then just use `sub_expr` as the condition expression and
  // swap the `then` and `else` branch. That is:
  // if !sub_expr { then_stmt } else { else_stmt } <=> if sub_expr { else_stmt } else { then_stmt }
  const PAstUnaryExpr* logical_not;
  if (else_bb != nullptr && (logical_not = get_logical_not_if_any(p_node->cond_expr)) != nullptr) {
    auto* condition = static_cast<llvm::Value*>(visit(logical_not->sub_expr));
    m_d->builder->CreateCondBr(condition, else_bb, then_bb); // then and else branches swapped
  } else {
    auto* condition = static_cast<llvm::Value*>(visit(p_node->cond_expr));
    m_d->builder->CreateCondBr(condition, then_bb, (else_bb != nullptr) ? else_bb : continue_bb);
  }

  // Then block:
  m_d->builder->SetInsertPoint(then_bb);
  visit(p_node->then_stmt);
  m_d->builder->CreateBr(continue_bb);

  // Else block:
  if (p_node->else_stmt != nullptr) {
    m_d->builder->SetInsertPoint(else_bb);
    visit(p_node->else_stmt);
    m_d->builder->CreateBr(continue_bb);
  }

  // Continue block:
  m_d->builder->SetInsertPoint(continue_bb);
  return nullptr;
}

void*
PCodeGenLLVM::visit_bool_literal(const PAstBoolLiteral* p_node)
{
  m_d->emit_location(p_node->get_source_range().begin);
  return llvm::ConstantInt::get(llvm::IntegerType::getInt1Ty(*m_d->llvm_ctx), p_node->value);
}

void*
PCodeGenLLVM::visit_int_literal(const PAstIntLiteral* p_node)
{
  m_d->emit_location(p_node->get_source_range().begin);
  auto* llvm_type = m_d->to_llvm_ty(p_node->get_type(m_d->ctx));
  const auto value = p_node->value;
  return llvm::ConstantInt::get(llvm_type, value);
}

void*
PCodeGenLLVM::visit_float_literal(const PAstFloatLiteral* p_node)
{
  m_d->emit_location(p_node->get_source_range().begin);
  auto* llvm_type = m_d->to_llvm_ty(p_node->get_type(m_d->ctx));
  const auto value = p_node->value;
  return llvm::ConstantFP::get(llvm_type, value);
}

void*
PCodeGenLLVM::visit_paren_expr(const PAstParenExpr* p_node)
{
  return visit(p_node->sub_expr);
}

void*
PCodeGenLLVM::visit_decl_ref_expr(const PAstDeclRefExpr* p_node)
{
  m_d->emit_location(p_node->get_source_range().begin);
  const auto it = m_d->decls.find(p_node->decl);
  assert(it != m_d->decls.end());
  return it->second;
}

void*
PCodeGenLLVM::visit_unary_expr(const PAstUnaryExpr* p_node)
{
  m_d->emit_location(p_node->get_source_range().begin);
  auto* value = static_cast<llvm::Value*>(visit(p_node->sub_expr));
  switch (p_node->opcode) {
    case P_UNARY_NEG: {
      auto* type = p_node->get_type(m_d->ctx);
      if (type->is_signed_int_ty()) {
        return m_d->builder->CreateNSWNeg(value);
      } else if (type->is_unsigned_int_ty()) {
        auto* llvm_type = m_d->to_llvm_ty(p_node->get_type(m_d->ctx));
        auto* zero = llvm::ConstantInt::get(llvm_type, 0);
        return m_d->builder->CreateSub(zero, value);
      } else if (type->is_float_ty()) {
        return m_d->builder->CreateFNeg(value);
      } else {
        assert(false && "invalid type for unary neg operator");
        return nullptr;
      }
    }

    case P_UNARY_NOT:
      return m_d->builder->CreateNot(value);

    case P_UNARY_DEREF: {
      auto* llvm_type = m_d->to_llvm_ty(p_node->get_type(m_d->ctx));
      return m_d->builder->CreateLoad(llvm_type, value);
    }

    default:
      assert(false && "unary operator not yet implemented");
      return nullptr;
  }
}

void*
PCodeGenLLVM::try_codegen_constant_log_and(PAstExpr* p_lhs, PAstExpr* p_rhs, bool p_is_and)
{
  // OPTIMIZATION: If one of the expressions is constant then simplify the
  // binary expression. So we avoid if possible generating branches, PHI
  // nodes and creating new basic blocks which may improve compile time (maybe).

  std::optional<bool> lhs_constant_value = p_lhs->eval_as_bool(m_d->ctx);
  if (lhs_constant_value.has_value()) {
    // true && expr <=> expr
    // true || expr <=> true
    // false && expr <=> false
    // false || expr <=> expr

    if (lhs_constant_value.value()) {
      if (p_is_and) // 'true && expr'
        return visit(p_rhs);
      else                                                                              // 'true || expr'
        return llvm::ConstantInt::get(llvm::IntegerType::getInt1Ty(*m_d->llvm_ctx), 1); // true
    } else {
      if (p_is_and)                                                                     // 'false && expr'
        return llvm::ConstantInt::get(llvm::IntegerType::getInt1Ty(*m_d->llvm_ctx), 0); // false
      else                                                                              // 'false || expr'
        return visit(p_rhs);
    }
  }

  std::optional<bool> rhs_constant_value = p_rhs->eval_as_bool(m_d->ctx);
  if (rhs_constant_value.has_value()) {
    // expr && true <=> expr
    // expr || true <=> true
    // expr && false <=> false
    // expr || false <=> expr

    if (lhs_constant_value.value()) {
      if (p_is_and) // 'expr && true'
        return visit(p_lhs);
      else                                                                              // 'expr || true'
        return llvm::ConstantInt::get(llvm::IntegerType::getInt1Ty(*m_d->llvm_ctx), 1); // true
    } else {
      if (p_is_and)                                                                     // 'expr && false'
        return llvm::ConstantInt::get(llvm::IntegerType::getInt1Ty(*m_d->llvm_ctx), 0); // false
      else                                                                              // 'expr || false'
        return visit(p_rhs);
    }
  }

  // The binary expression has no constant expression, and therefore we can
  // not optimize it (LLVM will do much more advanced optimizations later).
  return nullptr;
}

void*
PCodeGenLLVM::emit_log_and(PAstExpr* p_lhs, PAstExpr* p_rhs, bool p_is_and)
{
  // The code 'lhs && rhs' is equivalent to:
  //  1 | if (lhs) {
  //  2 |     return rhs;
  //  3 | } else {
  //  4 |     return false;
  //  5 | }
  //
  // Likewise, the code 'lhs || rhs' is equivalent to:
  //  1 | if (lhs) {
  //  2 |     return true;
  //  3 | } else {
  //  4 |     return rhs;
  //  5 | }

  // If either LHS or RHS can be constant evaluated then simplify the expression
  // to avoid creating branches, basic blocks and PHI nodes.
  if (void* value = try_codegen_constant_log_and(p_lhs, p_rhs, p_is_and); value != nullptr)
    return value;

  auto* lhs = static_cast<llvm::Value*>(visit(p_lhs));
  auto* func = m_d->get_function();

  llvm::BasicBlock* rhs_bb = llvm::BasicBlock::Create(*m_d->llvm_ctx, "", func);
  llvm::BasicBlock* continue_bb = llvm::BasicBlock::Create(*m_d->llvm_ctx, "", func);

  if (p_is_and) { // Implementing the '&&' operator
    m_d->builder->CreateCondBr(lhs, rhs_bb, continue_bb);
  } else { // Implementing the '||' operator
    m_d->builder->CreateCondBr(lhs, continue_bb, rhs_bb);
  }

  auto* bool_ty = llvm::IntegerType::getInt1Ty(*m_d->llvm_ctx);

  // Continue block:
  m_d->builder->SetInsertPoint(continue_bb);
  auto* phi = m_d->builder->CreatePHI(bool_ty, 2);

  // Constant block (either false or true depending on p_is_and):
  // As a matter of fact, this is not really a block but simply a constant.
  phi->addIncoming(llvm::ConstantInt::get(bool_ty, !p_is_and), m_d->builder->GetInsertBlock());

  // RHS block (`return rhs`).
  m_d->builder->SetInsertPoint(rhs_bb);
  phi->addIncoming(static_cast<llvm::Value*>(visit(p_rhs)), rhs_bb);
  rhs_bb = m_d->builder->GetInsertBlock();
  m_d->builder->CreateBr(continue_bb);

  return phi;
}

void*
PCodeGenLLVM::emit_trivial_bin_op(PType* p_type, void* p_llvm_lhs, void* p_llvm_rhs, PAstBinaryOp p_opcode)
{
  auto* lhs = static_cast<llvm::Value*>(p_llvm_lhs);
  auto* rhs = static_cast<llvm::Value*>(p_llvm_rhs);

  switch (p_opcode) {
    // Arithmetic binary operators
#define DISPATCH(p_signed_fn, p_unsigned_fn, p_float_fn)                                                               \
  if (p_type->is_signed_int_ty()) {                                                                                    \
    return m_d->builder->p_signed_fn(lhs, rhs);                                                                        \
  } else if (p_type->is_unsigned_int_ty()) {                                                                           \
    return m_d->builder->p_unsigned_fn(lhs, rhs);                                                                      \
  } else if (p_type->is_float_ty()) {                                                                                  \
    return m_d->builder->p_float_fn(lhs, rhs);                                                                         \
  } else {                                                                                                             \
    assert(false && "type not supported");                                                                             \
    return nullptr;                                                                                                    \
  }

    case P_BINARY_ADD:
    case P_BINARY_ASSIGN_ADD:
      DISPATCH(CreateNSWAdd, CreateAdd, CreateFAdd)
    case P_BINARY_SUB:
    case P_BINARY_ASSIGN_SUB:
      DISPATCH(CreateNSWSub, CreateSub, CreateFSub)
    case P_BINARY_MUL:
    case P_BINARY_ASSIGN_MUL:
      DISPATCH(CreateNSWMul, CreateMul, CreateFMul)
    case P_BINARY_DIV:
    case P_BINARY_ASSIGN_DIV:
      DISPATCH(CreateSDiv, CreateUDiv, CreateFDiv)
    case P_BINARY_MOD:
    case P_BINARY_ASSIGN_MOD:
      DISPATCH(CreateSRem, CreateURem, CreateFRem)

#undef DISPATCH

      // Integer restricted binary operators
#define DISPATCH(p_signed_fn, p_unsigned_fn)                                                                           \
  if (p_type->is_signed_int_ty()) {                                                                                    \
    return m_d->builder->p_signed_fn(lhs, rhs);                                                                        \
  } else if (p_type->is_unsigned_int_ty()) {                                                                           \
    return m_d->builder->p_unsigned_fn(lhs, rhs);                                                                      \
  } else {                                                                                                             \
    assert(false && "type not supported");                                                                             \
    return nullptr;                                                                                                    \
  }

    case P_BINARY_SHL:
    case P_BINARY_ASSIGN_SHL:
      DISPATCH(CreateShl, CreateShl)
    case P_BINARY_SHR:
    case P_BINARY_ASSIGN_SHR:
      DISPATCH(CreateAShr, CreateLShr)
    case P_BINARY_BIT_AND:
    case P_BINARY_ASSIGN_BIT_AND:
      DISPATCH(CreateAnd, CreateAnd)
    case P_BINARY_BIT_XOR:
    case P_BINARY_ASSIGN_BIT_XOR:
      DISPATCH(CreateXor, CreateXor)
    case P_BINARY_BIT_OR:
    case P_BINARY_ASSIGN_BIT_OR:
      DISPATCH(CreateOr, CreateOr)

#undef DISPATCH

      // Comparison binary operators
#define DISPATCH(p_signed_op, p_unsigned_op, p_float_op)                                                               \
  if (p_type->is_signed_int_ty()) {                                                                                    \
    return m_d->builder->CreateICmp##p_signed_op(lhs, rhs);                                                            \
  } else if (p_type->is_unsigned_int_ty()) {                                                                           \
    return m_d->builder->CreateICmp##p_unsigned_op(lhs, rhs);                                                          \
  } else if (p_type->is_float_ty()) {                                                                                  \
    return m_d->builder->CreateFCmp##p_float_op(lhs, rhs);                                                             \
  } else {                                                                                                             \
    assert(false && "type not supported");                                                                             \
    return nullptr;                                                                                                    \
  }

    case P_BINARY_LT:
      DISPATCH(SLT, ULT, OLT)
    case P_BINARY_LE:
      DISPATCH(SLE, ULE, OLE)
    case P_BINARY_GT:
      DISPATCH(SGT, UGT, OGT)
    case P_BINARY_GE:
      DISPATCH(SGE, UGE, OGE)
    case P_BINARY_EQ:
      DISPATCH(EQ, EQ, OEQ)
    case P_BINARY_NE:
      DISPATCH(NE, NE, UNE)

#undef DISPATCH
    default:
      assert(false && "binary operator not yet implemented");
      return nullptr;
  }
}

void*
PCodeGenLLVM::visit_binary_expr(const PAstBinaryExpr* p_node)
{
  m_d->emit_location(p_node->get_source_range().begin);
  const auto opcode = p_node->opcode;

  if (opcode == P_BINARY_LOG_AND || opcode == P_BINARY_LOG_OR)
    return emit_log_and(p_node->lhs, p_node->rhs, (opcode == P_BINARY_LOG_AND));

  auto* lhs = static_cast<llvm::Value*>(visit(p_node->lhs));
  auto* rhs = static_cast<llvm::Value*>(visit(p_node->rhs));
  if (p_binop_is_assignment(opcode)) {
    if (opcode == P_BINARY_ASSIGN) {
      return m_d->builder->CreateStore(rhs, lhs);
    } else {
      auto* lhs_type = p_node->lhs->get_type(m_d->ctx);
      auto* type = m_d->to_llvm_ty(lhs_type);
      auto* lhs_value = m_d->builder->CreateLoad(type, lhs);

      auto* result = static_cast<llvm::Value*>(emit_trivial_bin_op(lhs_type, lhs_value, rhs, opcode));
      return m_d->builder->CreateStore(result, lhs);
    }
  } else {
    auto* lhs_type = p_node->lhs->get_type(m_d->ctx);
    return emit_trivial_bin_op(lhs_type, lhs, rhs, opcode);
  }
}

void*
PCodeGenLLVM::visit_member_expr(const PAstMemberExpr* p_node)
{
  m_d->emit_location(p_node->get_source_range().begin);
  auto* struct_ty = m_d->to_llvm_ty(p_node->base_expr->get_type(m_d->ctx));
  auto* base_expr = static_cast<llvm::Value*>(visit(p_node->base_expr));
  return m_d->builder->CreateStructGEP(struct_ty, base_expr, p_node->member->get_index_in_parent_fields());
}

void*
PCodeGenLLVM::visit_call_expr(const PAstCallExpr* p_node)
{
  m_d->emit_location(p_node->get_source_range().begin);
  auto* callee_type = m_d->to_llvm_ty(p_node->callee->get_type(m_d->ctx));
  assert(callee_type->isFunctionTy());
  auto* callee = static_cast<llvm::Value*>(visit(p_node->callee));

  std::vector<llvm::Value*> args(p_node->args.size());
  for (size_t i = 0; i < p_node->args.size(); ++i) {
    args[i] = static_cast<llvm::Value*>(visit(p_node->args[i]));
  }

  return m_d->builder->CreateCall(static_cast<llvm::FunctionType*>(callee_type), callee, args);
}

void*
PCodeGenLLVM::visit_cast_expr(const PAstCastExpr* p_node)
{
  m_d->emit_location(p_node->get_source_range().begin);
  auto* source_ty = p_node->sub_expr->get_type(m_d->ctx);
  auto* sub_expr = static_cast<llvm::Value*>(visit(p_node->sub_expr));
  auto* llvm_target_ty = m_d->to_llvm_ty(p_node->target_ty);
  switch (p_node->cast_kind) {
    case P_CAST_NOOP:
      return sub_expr;
    case P_CAST_INT2INT:
      return m_d->builder->CreateIntCast(sub_expr, llvm_target_ty, p_node->target_ty->is_signed_int_ty());
    case P_CAST_INT2FLOAT:
      if (source_ty->is_unsigned_int_ty())
        return m_d->builder->CreateUIToFP(sub_expr, llvm_target_ty);
      else
        return m_d->builder->CreateSIToFP(sub_expr, llvm_target_ty);
    case P_CAST_FLOAT2FLOAT:
      return m_d->builder->CreateFPCast(sub_expr, llvm_target_ty);
    case P_CAST_FLOAT2INT:
      if (p_node->target_ty->is_unsigned_int_ty())
        return m_d->builder->CreateFPToUI(sub_expr, llvm_target_ty);
      else
        return m_d->builder->CreateFPToSI(sub_expr, llvm_target_ty);
    case P_CAST_BOOL2INT:
      return m_d->builder->CreateZExt(sub_expr, llvm_target_ty);
    case P_CAST_BOOL2FLOAT:
      return m_d->builder->CreateUIToFP(sub_expr, llvm_target_ty);
    default:
      assert(false && "cast operation not yet implemented");
      return nullptr;
  }
}

void*
PCodeGenLLVM::visit_struct_expr(const PAstStructExpr* p_node)
{
  auto* struct_ty = m_d->to_llvm_ty(p_node->get_struct_decl()->get_type());
  auto* struct_ptr = m_d->insert_alloc_in_entry_bb(struct_ty);

  for (auto* field : p_node->get_fields()) {
    auto* field_decl = field->get_field_decl();
    m_d->emit_location(field->get_expr_range().begin);
    auto* field_ptr = m_d->builder->CreateStructGEP(struct_ty, struct_ptr, field_decl->get_index_in_parent_fields());
    auto* expr = static_cast<llvm::Value*>(visit(field->get_expr()));
    m_d->builder->CreateStore(expr, field_ptr);
  }

  m_d->emit_location(p_node->get_source_range().begin);
  return m_d->builder->CreateLoad(struct_ty, struct_ptr);
}

void*
PCodeGenLLVM::visit_l2rvalue_expr(const PAstL2RValueExpr* p_node)
{
  m_d->emit_location(p_node->get_source_range().begin);
  auto* type = m_d->to_llvm_ty(p_node->get_type(m_d->ctx));
  auto* ptr = static_cast<llvm::Value*>(visit(p_node->sub_expr));
  return m_d->builder->CreateLoad(type, ptr);
}

void*
PCodeGenLLVM::visit_func_decl(const PFunctionDecl* p_node)
{
  // Do not waste time generating code when is not needed.
  if (!p_node->is_used() && p_node->is_extern() && !p_node->has_body())
    return nullptr;

  auto* func_ty = m_d->to_llvm_ty(p_node->get_type());
  assert(func_ty->isFunctionTy());
  auto func_callee =
    m_d->llvm_module->getOrInsertFunction(to_str_ref(p_node->get_name()), static_cast<llvm::FunctionType*>(func_ty));

  auto* func = llvm::cast<llvm::Function>(func_callee.getCallee());
  assert(func != nullptr);

  if (p_node->is_extern()) {
    func->setLinkage(llvm::Function::ExternalLinkage);
  }

  std::string_view abi;
  if (p_node->has_abi())
    abi = p_node->get_abi();
  else if (p_node->is_extern())
    abi = "C";

  if (!abi.empty()) {
    if (abi == "C") {
      func->setCallingConv(llvm::CallingConv::C);
    } else if (abi == "stdcall") {
      func->setCallingConv(llvm::CallingConv::X86_StdCall);
    } else if (abi == "fastcall") {
      func->setCallingConv(llvm::CallingConv::X86_FastCall);
    } else if (abi == "vectorcall") {
      func->setCallingConv(llvm::CallingConv::X86_VectorCall);
    } else {
      assert(false && "unreachable");
    }
  }

  m_d->decls.insert({ p_node, func });

  // Generate debug info only for functions with a definition.
  bool generate_debug_info = p_node->has_body();

  llvm::DISubprogram* debug_subprogram;
  if (generate_debug_info) {
    debug_subprogram =
      m_d->debug_builder->createFunction(m_d->debug_file,
                                         to_str_ref(p_node->get_name()),
                                         "",
                                         m_d->debug_file,
                                         0,
                                         static_cast<llvm::DISubroutineType*>(m_d->to_debug_ty(p_node->get_type())),
                                         0,
                                         llvm::DINode::FlagPrototyped,
                                         llvm::DISubprogram::SPFlagDefinition);
    func->setSubprogram(debug_subprogram);
  }

  // Function body generation.
  if (p_node->has_body()) {
    m_d->lexical_blocks.push(debug_subprogram);
    m_d->reset_location();

    auto* entry_bb = llvm::BasicBlock::Create(*m_d->llvm_ctx, "", func);
    m_d->builder->SetInsertPoint(entry_bb);

    // Allocate all parameters on the stack so the user can modify them.
    for (size_t i = 0; i < p_node->params.size(); ++i) {
      auto* param = p_node->params[i];
      auto* param_ty = m_d->to_llvm_ty(param->get_type());
      auto* param_addr = m_d->builder->CreateAlloca(param_ty);
      m_d->builder->CreateStore(func->getArg(i), param_addr);
      m_d->decls.insert({ param, param_addr });

      // Generate debug info for the parameter
      auto* param_info = m_d->debug_builder->createParameterVariable(
        debug_subprogram, to_str_ref(param->get_name()), i, nullptr, 0, m_d->to_debug_ty(param->get_type()));
      m_d->debug_builder->insertDeclare(param_addr,
                                        param_info,
                                        m_d->debug_builder->createExpression(),
                                        m_d->get_di_location(param->source_range.begin),
                                        m_d->builder->GetInsertBlock());
    }

    m_d->emit_location(p_node->get_body()->get_source_range().begin);
    visit(p_node->get_body());
    m_d->finish_func_codegen();
    m_d->lexical_blocks.pop();
  }

  return nullptr;
}

void*
PCodeGenLLVM::visit_var_decl(const PVarDecl* p_node)
{
  auto* type = m_d->to_llvm_ty(p_node->get_type());
  auto* ptr = m_d->insert_alloc_in_entry_bb(type);

  m_d->decls.insert({ p_node, ptr });

  auto* var_info = m_d->emit_var_info(p_node);
  m_d->debug_builder->insertDeclare(ptr,
                                    var_info,
                                    m_d->debug_builder->createExpression(),
                                    m_d->get_di_location(p_node->source_range.begin),
                                    m_d->builder->GetInsertBlock());

  if (p_node->init_expr != nullptr) {
    auto* init_value = static_cast<llvm::Value*>(visit(p_node->init_expr));
    m_d->builder->CreateStore(init_value, ptr);
  }

  return nullptr;
}
