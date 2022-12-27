#include "sema.hxx"

#include "utils/bump_allocator.hxx"
#include "utils/diag.hxx"

#include <cassert>
#include <cstdlib>
#include <cstring>

PSema::PSema(PContext& p_context)
  : m_context(p_context)
{
}

PSema::~PSema()
{
  assert(m_current_scope == nullptr);

  PScope* scope = m_current_scope;
  while (scope != nullptr) {
    PScope* parent = scope->parent_scope;
    delete scope;
    scope = parent;
  }
}

void
PSema::push_scope(PScopeFlags p_flags)
{
  m_current_scope = new PScope(m_current_scope, p_flags);
}

void
PSema::pop_scope()
{
  assert(m_current_scope != nullptr);

  PScope* parent = m_current_scope->parent_scope;
  delete m_current_scope;
  m_current_scope = parent;
}

PSymbol*
PSema::lookup(PIdentifierInfo* p_name) const
{
  assert(p_name != nullptr);

  PScope* scope = m_current_scope;
  do {
    PSymbol* symbol = p_scope_local_lookup(scope, p_name);
    if (symbol != nullptr)
      return symbol;
    scope = scope->parent_scope;
  } while (scope != nullptr);

  return nullptr;
}

PSymbol*
PSema::local_lookup(PIdentifierInfo* p_name) const
{
  assert(p_name != nullptr);
  return p_scope_local_lookup(m_current_scope, p_name);
}

PAstBoolLiteral*
PSema::act_on_bool_literal(bool p_value, PSourceRange p_src_range)
{
  return m_context.new_object<PAstBoolLiteral>(p_value, p_src_range);
}

PType*
PSema::get_type_for_int_literal_suffix(PIntLiteralSuffix p_suffix, uintmax_t p_value) const
{
  switch (p_suffix) {
    case P_ILS_I8:
      return m_context.get_i8_ty();
    case P_ILS_I16:
      return m_context.get_i16_ty();
    case P_ILS_I32:
      return m_context.get_i32_ty();
    case P_ILS_I64:
      return m_context.get_i64_ty();
    case P_ILS_U8:
      return m_context.get_u8_ty();
    case P_ILS_U16:
      return m_context.get_u16_ty();
    case P_ILS_U32:
      return m_context.get_u32_ty();
    case P_ILS_U64:
      return m_context.get_u64_ty();
    case P_ILS_NO_SUFFIX:
      if (p_value > INT64_MAX) {
        return m_context.get_u64_ty();
      } else if (p_value > INT32_MAX) {
        return m_context.get_i64_ty();
      } else {
        return m_context.get_i32_ty();
      }
    default:
      HEDLEY_UNREACHABLE_RETURN(nullptr);
  }
}

bool
PSema::is_int_type_big_enough(uintmax_t p_value, PType* p_type) const
{
  assert(p_type != nullptr && p_type->is_int_ty());

  switch (p_type->get_kind()) {
    case P_TK_I8:
      return p_value <= INT8_MAX;
    case P_TK_I16:
      return p_value <= INT16_MAX;
    case P_TK_I32:
      return p_value <= INT32_MAX;
    case P_TK_I64:
      return p_value <= INT64_MAX;
    case P_TK_U8:
      return p_value <= UINT8_MAX;
    case P_TK_U16:
      return p_value <= UINT16_MAX;
    case P_TK_U32:
      return p_value <= UINT32_MAX;
    case P_TK_U64:
      return p_value <= UINT64_MAX;
    default:
      assert(false && "not an integer type");
      return false;
  }
}

PAstIntLiteral*
PSema::act_on_int_literal(uintmax_t p_value, PIntLiteralSuffix p_suffix, PSourceRange p_src_range)
{
  PType* type = get_type_for_int_literal_suffix(p_suffix, p_value);

  if (!is_int_type_big_enough(p_value, type)) {
    PDiag* d = diag_at(P_DK_err_int_literal_too_large, p_src_range.begin);
    diag_add_arg_type(d, type);
    diag_add_source_range(d, p_src_range);
    diag_flush(d);
    p_value = 0; // set a value to recover
  }

  return m_context.new_object<PAstIntLiteral>(p_value, type, p_src_range);
}

PType*
PSema::get_type_for_float_literal_suffix(PFloatLiteralSuffix p_suffix, double p_value) const
{
  switch (p_suffix) {
    case P_FLS_NO_SUFFIX:
      // The default float type.
      return m_context.get_f64_ty();
    case P_FLS_F32:
      return m_context.get_f32_ty();
    case P_FLS_F64:
      return m_context.get_f64_ty();
    default:
      HEDLEY_UNREACHABLE_RETURN(nullptr);
  }
}

PAstFloatLiteral*
PSema::act_on_float_literal(double p_value, PFloatLiteralSuffix p_suffix, PSourceRange p_src_range)
{
  PType* type = get_type_for_float_literal_suffix(p_suffix, p_value);
  return m_context.new_object<PAstFloatLiteral>(p_value, type, p_src_range);
}

PAstParenExpr*
PSema::act_on_paren_expr(PAstExpr* p_sub_expr, PSourceRange p_src_range)
{
  assert(p_sub_expr != nullptr);
  return m_context.new_object<PAstParenExpr>(p_sub_expr, p_src_range);
}

/* Returns true if p_from is trivially convertible to p_to.
 * Most time this is equivalent to p_from == p_to, unless
 * if one of the types is a generic int/float. */
static bool
are_types_compatible(PType* p_from, PType* p_to)
{
  p_from = p_from->get_canonical_ty();
  p_to = p_to->get_canonical_ty();

  return p_from == p_to;
}

PAstLetStmt*
PSema::act_on_let_stmt(PVarDecl** p_decls, size_t p_decl_count, PSourceRange p_src_range)
{
  assert(p_decls != nullptr && p_decl_count > 0);

  auto* raw_decls = m_context.alloc_object<PVarDecl*>(p_decl_count);
  std::copy(p_decls, p_decls + p_decl_count, raw_decls);

  auto* node = m_context.new_object<PAstLetStmt>(raw_decls, p_decl_count);
  return node;
}

PAstLetStmt*
PSema::act_on_let_stmt(const std::vector<PVarDecl*>& p_decls, PSourceRange p_src_range)
{
  return act_on_let_stmt(const_cast<PVarDecl**>(p_decls.data()), p_decls.size(), p_src_range);
}

PAstBreakStmt*
PSema::act_on_break_stmt(PSourceRange p_src_range, PSourceRange p_break_key_range)
{
  PScope* break_scope = find_nearest_scope_with_flag(P_SF_BREAK);
  if (break_scope == nullptr) {
    PDiag* d = diag_at(P_DK_err_break_or_continue_outside_of_loop, p_break_key_range.begin);
    diag_add_arg_str(d, "break");
    diag_add_source_range(d, p_break_key_range);
    diag_flush(d);
    return nullptr;
  }

  return m_context.new_object<PAstBreakStmt>(p_src_range);
}

PAstContinueStmt*
PSema::act_on_continue_stmt(PSourceRange p_src_range, PSourceRange p_cont_key_range)
{
  PScope* continue_scope = find_nearest_scope_with_flag(P_SF_CONTINUE);
  if (continue_scope == nullptr) {
    PDiag* d = diag_at(P_DK_err_break_or_continue_outside_of_loop, p_cont_key_range.begin);
    diag_add_arg_str(d, "continue");
    diag_add_source_range(d, p_cont_key_range);
    diag_flush(d);
    return nullptr;
  }

  return m_context.new_object<PAstContinueStmt>(p_src_range);
}

bool
PSema::is_compatible_with_ret_ty(PType* p_type) const
{
  assert(p_type != nullptr);
  return are_types_compatible(p_type, m_curr_func_type->get_ret_ty());
}

PAstReturnStmt*
PSema::act_on_return_stmt(PAstExpr* p_ret_expr, PSourceRange p_src_range, PSourceLocation p_semi_loc)
{
  // Check if the return expression type and the current function return type are compatible
  // and if not then emit a diagnostic.
  PType* type = (p_ret_expr != nullptr) ? p_ret_expr->get_type(m_context) : m_context.get_void_ty();
  if (!is_compatible_with_ret_ty(type)) {
    PSourceLocation diag_loc = (p_ret_expr != nullptr) ? p_ret_expr->get_source_range().begin : p_semi_loc;

    PDiag* d = diag_at(P_DK_err_expected_type, diag_loc);
    diag_add_arg_type(d, m_curr_func_type->get_ret_ty());
    diag_add_arg_type(d, type);

    if (p_ret_expr != nullptr)
      diag_add_source_range(d, p_ret_expr->get_source_range());

    diag_flush(d);
  }

  auto* node = m_context.new_object<PAstReturnStmt>(p_ret_expr, p_src_range);
  return node;
}

void
PSema::check_condition_expr(PAstExpr* p_cond_expr) const
{
  // TODO: add warnings about suspicious use of some operators in conditions
  //       like '|=' instead of '!=' or '=' instead of '=='.

  PType* cond_type = p_cond_expr->get_type(m_context);
  if (cond_type->is_bool_ty())
    return;

  PDiag* d = diag_at(P_DK_err_expected_type, p_cond_expr->get_source_range().begin);
  diag_add_arg_type(d, m_context.get_bool_ty());
  diag_add_arg_type(d, cond_type);
  diag_add_source_range(d, p_cond_expr->get_source_range());
  diag_flush(d);
}

void
PSema::act_before_loop_body_common()
{
  push_scope(static_cast<PScopeFlags>(P_SF_BREAK | P_SF_CONTINUE));
}

void
PSema::act_before_while_stmt_body()
{
  act_before_loop_body_common();
}

PAstWhileStmt*
PSema::act_on_while_stmt(PAstExpr* p_cond_expr, PAst* p_body, PSourceRange p_src_range)
{
  assert(p_cond_expr != nullptr && p_body != nullptr);

  pop_scope();

  check_condition_expr(p_cond_expr);
  // FIXME: remove the cast to PAstExpr*
  p_cond_expr = (PAstExpr*)convert_to_rvalue(p_cond_expr);

  auto* node = m_context.new_object<PAstWhileStmt>(p_cond_expr, p_body, p_src_range);
  return node;
}

void
PSema::act_before_loop_stmt_body()
{
  act_before_loop_body_common();
}

PAstLoopStmt*
PSema::act_on_loop_stmt(PAst* p_body, PSourceRange p_src_range)
{
  assert(p_body != nullptr);

  pop_scope();

  auto* node = m_context.new_object<PAstLoopStmt>(p_body, p_src_range);
  return node;
}

PAstIfStmt*
PSema::act_on_if_stmt(PAstExpr* p_cond_expr, PAst* p_then_stmt, PAst* p_else_stmt, PSourceRange p_src_range)
{
  assert(p_cond_expr != nullptr && p_then_stmt != nullptr);

  check_condition_expr(p_cond_expr);
  p_cond_expr = convert_to_rvalue(p_cond_expr);

  auto* node = m_context.new_object<PAstIfStmt>(p_cond_expr, p_then_stmt, p_else_stmt, p_src_range);
  return node;
}

PAstDeclRefExpr*
PSema::act_on_decl_ref_expr(PIdentifierInfo* p_name, PSourceRange p_src_range)
{
  assert(p_name != nullptr);

  PSymbol* symbol = lookup(p_name);
  if (symbol == nullptr) {
    PDiag* d = diag_at(P_DK_err_use_undeclared_ident, p_src_range.begin);
    diag_add_arg_ident(d, p_name);
    diag_add_source_range(d, p_src_range);
    diag_flush(d);
    return nullptr;
  }

  assert(symbol->decl != nullptr);
  symbol->decl->used = true;

  auto* node = m_context.new_object<PAstDeclRefExpr>(symbol->decl, p_src_range);
  return node;
}

PAstUnaryExpr*
PSema::act_on_unary_expr(PAstExpr* p_sub_expr, PAstUnaryOp p_opcode, PSourceRange p_src_range, PSourceLocation p_op_loc)
{
  assert(p_sub_expr != nullptr);

  PType* result_type = p_sub_expr->get_type(m_context);
  switch (p_opcode) {
    case P_UNARY_NEG: // '-' operator
      if (!result_type->is_float_ty() && !result_type->is_signed_int_ty()) {
        PDiag* d = diag_at(P_DK_err_cannot_apply_unary_op, p_op_loc);
        diag_add_arg_char(d, '-');
        diag_add_arg_type(d, result_type);
        diag_add_source_range(d, p_sub_expr->get_source_range());
        diag_flush(d);
        return nullptr;
      }

      p_sub_expr = (p_sub_expr);
      break;
    case P_UNARY_NOT: // logical and bitwise '!' operator
      if (!result_type->is_bool_ty() && !result_type->is_int_ty()) {
        PDiag* d = diag_at(P_DK_err_cannot_apply_unary_op, p_op_loc);
        diag_add_arg_char(d, '!');
        diag_add_arg_type(d, result_type);
        diag_add_source_range(d, p_sub_expr->get_source_range());
        diag_flush(d);
        return nullptr;
      }

      p_sub_expr = convert_to_rvalue(p_sub_expr);
      break;
    case P_UNARY_ADDRESS_OF: // '&' operator
      if (p_sub_expr->is_rvalue()) {
        PDiag* d = diag_at(P_DK_err_could_not_take_addr_rvalue, p_op_loc);
        diag_add_arg_type(d, result_type);
        diag_add_source_range(d, p_sub_expr->get_source_range());
        diag_flush(d);
        return nullptr;
      }

      result_type = m_context.get_pointer_ty(result_type);
      break;
    case P_UNARY_DEREF: // '*' operator
      if (!result_type->is_pointer_ty()) {
        PDiag* d = diag_at(P_DK_err_indirection_requires_ptr, p_op_loc);
        diag_add_arg_type(d, result_type);
        diag_add_source_range(d, p_sub_expr->get_source_range());
        diag_flush(d);
        return nullptr;
      }

      result_type = result_type->as<PPointerType>()->get_element_ty();
      break;
    default:
      HEDLEY_UNREACHABLE_RETURN(nullptr);
  }

  auto* node = m_context.new_object<PAstUnaryExpr>(p_sub_expr, result_type, p_opcode, p_src_range);
  return node;
}

PAstBinaryExpr*
PSema::act_on_binary_expr(PAstExpr* p_lhs,
                          PAstExpr* p_rhs,
                          PAstBinaryOp p_opcode,
                          PSourceRange p_src_range,
                          PSourceLocation p_op_loc)
{
  assert(p_lhs != nullptr && p_rhs != nullptr);

  PType* lhs_type = p_lhs->get_type(m_context);
  PType* rhs_type = p_rhs->get_type(m_context);

  PType* result_type;
  switch (p_opcode) {
    case P_BINARY_LOG_AND:
    case P_BINARY_LOG_OR: {
      result_type = m_context.get_bool_ty();

      if (!lhs_type->is_bool_ty()) {
        PDiag* d = diag_at(P_DK_err_expected_type, p_op_loc);
        diag_add_arg_type(d, m_context.get_bool_ty());
        diag_add_arg_type(d, lhs_type);
        diag_add_source_range(d, p_lhs->get_source_range());
        diag_add_source_range(d, p_rhs->get_source_range());
        diag_flush(d);
      }

      if (!rhs_type->is_bool_ty()) {
        PDiag* d = diag_at(P_DK_err_expected_type, p_op_loc);
        diag_add_arg_type(d, m_context.get_bool_ty());
        diag_add_arg_type(d, rhs_type);
        diag_add_source_range(d, p_lhs->get_source_range());
        diag_add_source_range(d, p_rhs->get_source_range());
        diag_flush(d);
      }

    } break;

    case P_BINARY_EQ:
    case P_BINARY_NE:
    case P_BINARY_LT:
    case P_BINARY_LE:
    case P_BINARY_GT:
    case P_BINARY_GE:
      if (!are_types_compatible(lhs_type, rhs_type)) {
        PDiag* d = diag_at(P_DK_err_cannot_apply_bin_op_generic, p_op_loc);
        diag_add_arg_str(d, p_get_spelling(p_opcode));
        diag_add_arg_type(d, lhs_type);
        diag_add_arg_type(d, rhs_type);
        diag_add_source_range(d, p_lhs->get_source_range());
        diag_add_source_range(d, p_rhs->get_source_range());
        diag_add_source_caret(d, p_op_loc);
        diag_flush(d);
      }

      result_type = m_context.get_bool_ty();
      break;

    case P_BINARY_ASSIGN_BIT_AND:
    case P_BINARY_ASSIGN_BIT_XOR:
    case P_BINARY_ASSIGN_BIT_OR:
    case P_BINARY_ASSIGN_SHL:
    case P_BINARY_ASSIGN_SHR: {
      result_type = lhs_type;

      if (!lhs_type->is_int_ty()) {
        PDiag* d = diag_at(P_DK_err_cannot_apply_assign_op, p_op_loc);
        const char* spelling = p_get_spelling(p_opcode);
        diag_add_arg_str(d, spelling);
        diag_add_arg_type(d, lhs_type);
        diag_add_source_caret(d, p_op_loc);
        diag_flush(d);
        break;
      }

      if (!are_types_compatible(rhs_type, lhs_type)) {
        PDiag* d = diag_at(P_DK_err_expected_type, p_op_loc);
        diag_add_arg_type(d, lhs_type);
        diag_add_arg_type(d, rhs_type);
        diag_add_source_range(d, p_rhs->get_source_range());
        diag_flush(d);
        break;
      }
    } break;
    case P_BINARY_BIT_AND:
    case P_BINARY_BIT_XOR:
    case P_BINARY_BIT_OR:
    case P_BINARY_SHL:
    case P_BINARY_SHR: {
      result_type = lhs_type;

      if (!lhs_type->is_int_ty()) {
        PDiag* d = diag_at(P_DK_err_expected_int_type, p_op_loc);
        diag_add_arg_type(d, lhs_type);
        diag_add_source_range(d, p_lhs->get_source_range());
        diag_flush(d);
      }

      if (!rhs_type->is_int_ty()) {
        PDiag* d = diag_at(P_DK_err_expected_int_type, p_op_loc);
        diag_add_arg_type(d, rhs_type);
        diag_add_source_range(d, p_rhs->get_source_range());
        diag_flush(d);
      }
    } break;
    default:
      result_type = lhs_type;

      if (!are_types_compatible(lhs_type, rhs_type) || !lhs_type->is_arithmetic_ty()) {
        PDiagKind diag_kind = P_DK_err_cannot_apply_bin_op_generic;
        switch (p_opcode) {
          case P_BINARY_ADD:
            diag_kind = P_DK_err_cannot_add;
            break;
          case P_BINARY_ASSIGN_ADD:
            diag_kind = P_DK_err_cannot_add_assign;
            break;
          case P_BINARY_SUB:
            diag_kind = P_DK_err_cannot_sub;
            break;
          case P_BINARY_ASSIGN_SUB:
            diag_kind = P_DK_err_cannot_sub_assign;
            break;
          case P_BINARY_MUL:
            diag_kind = P_DK_err_cannot_mul;
            break;
          case P_BINARY_ASSIGN_MUL:
            diag_kind = P_DK_err_cannot_mul_assign;
            break;
          case P_BINARY_DIV:
            diag_kind = P_DK_err_cannot_div;
            break;
          case P_BINARY_ASSIGN_DIV:
            diag_kind = P_DK_err_cannot_div_assign;
            break;
          default:
            break;
        }

        PDiag* d = diag_at(diag_kind, p_op_loc);
        if (diag_kind == P_DK_err_cannot_apply_bin_op_generic) {
          const char* spelling = p_get_spelling(p_opcode);
          diag_add_arg_str(d, spelling);
        }

        diag_add_arg_type(d, lhs_type);
        diag_add_arg_type(d, rhs_type);
        diag_add_source_range(d, p_lhs->get_source_range());
        diag_add_source_range(d, p_rhs->get_source_range());
        diag_add_source_caret(d, p_op_loc);
        diag_flush(d);
      }

      break;
  }

  assert(result_type != nullptr);

  if (!p_binop_is_assignment(p_opcode)) {
    p_lhs = convert_to_rvalue(p_lhs);
  }

  p_rhs = convert_to_rvalue(p_rhs);

  auto* node = m_context.new_object<PAstBinaryExpr>(p_lhs, p_rhs, result_type, p_opcode, p_src_range);
  return node;
}

void
PSema::check_call_args(PFunctionType* p_callee_ty,
                       PAstExpr** p_args,
                       size_t p_arg_count,
                       PFunctionDecl* p_func_decl,
                       PSourceLocation p_lparen_loc)
{
  const size_t required_param_count = p_callee_ty->get_param_count();
  if (p_arg_count != required_param_count) {
    PDiag* d =
      diag_at((p_arg_count < required_param_count) ? P_DK_err_too_few_args : P_DK_err_too_many_args, p_lparen_loc);
    if (p_func_decl != nullptr)
      diag_add_arg_type_with_name_hint(d, p_callee_ty, p_func_decl->name);
    else
      diag_add_arg_type(d, p_callee_ty);
    diag_add_source_caret(d, p_lparen_loc);
    diag_flush(d);
  }

  PType** args_expected_type = p_callee_ty->get_params();
  for (size_t i = 0; i < std::min(p_arg_count, required_param_count); ++i) {
    if (p_args[i] == nullptr)
      continue;

    PType* arg_type = p_args[i]->get_type(m_context);
    if (!are_types_compatible(arg_type, args_expected_type[i])) {
      PDiag* d = diag_at(P_DK_err_expected_type, p_args[i]->get_source_range().begin);
      diag_add_arg_type(d, args_expected_type[i]);
      diag_add_arg_type(d, arg_type);
      diag_add_source_range(d, p_args[i]->get_source_range());
      diag_flush(d);
    }
  }
}

PAstCallExpr*
PSema::act_on_call_expr(PAstExpr* p_callee,
                        PAstExpr** p_args,
                        size_t p_arg_count,
                        PSourceRange p_src_range,
                        PSourceLocation p_lparen_loc)
{
  assert(p_callee != nullptr && (p_args != nullptr || p_arg_count == 0));

  auto* callee_decl = try_get_ref_decl(p_callee);
  auto* callee_ty = p_callee->get_type(m_context);
  if (!callee_ty->is_function_ty()) {
    PDiag* d;

    if (callee_decl != nullptr) {
      d = diag_at(P_DK_err_cannot_be_used_as_function, p_lparen_loc);
      diag_add_arg_ident(d, P_DECL_GET_NAME(callee_decl));
    } else {
      d = diag_at(P_DK_err_expr_cannot_be_used_as_function, p_lparen_loc);
    }

    diag_add_source_range(d, p_callee->get_source_range());
    diag_add_source_caret(d, p_lparen_loc);
    diag_flush(d);
    return nullptr;
  }

  check_call_args(callee_ty->as<PFunctionType>(), p_args, p_arg_count, (PFunctionDecl*)callee_decl, p_lparen_loc);

  auto** raw_args = m_context.alloc_object<PAstExpr*>(p_arg_count);
  std::copy(p_args, p_args + p_arg_count, raw_args);

  auto* node = m_context.new_object<PAstCallExpr>(p_callee, raw_args, p_arg_count, p_src_range);

  // Convert all lvalue arguments to rvalue
  for (size_t i = 0; i < p_arg_count; ++i) {
    node->args[i] = convert_to_rvalue(node->args[i]);
  }

  return node;
}

PAstCallExpr*
PSema::act_on_call_expr(PAstExpr* p_callee,
                        const std::vector<PAstExpr*>& p_args,
                        PSourceRange p_src_range,
                        PSourceLocation p_lparen_loc)
{
  return act_on_call_expr(p_callee, const_cast<PAstExpr**>(p_args.data()), p_args.size(), p_src_range, p_lparen_loc);
}

/* Classify {int} -> p_to_type cast. */
static PAstCastKind
classify_int_cast(PType* p_from_type, PType* p_to_type)
{
  if (p_to_type->is_float_ty())
    return P_CAST_INT2FLOAT;

  if (p_to_type->is_int_ty()) {
    int from_bitwidth = p_type_get_bitwidth(p_from_type);
    int to_bitwidth = p_type_get_bitwidth(p_to_type);
    if (from_bitwidth != to_bitwidth) {
      return P_CAST_INT2INT;
    }

    // Signed <-> Unsigned integer conversion.
    return P_CAST_NOOP;
  }

  return P_CAST_INVALID;
}

/* Classify {float} -> p_to_type cast. */
static PAstCastKind
classify_float_cast(PType* p_to_type)
{
  if (p_to_type->is_int_ty())
    return P_CAST_FLOAT2INT;

  if (p_to_type->is_float_ty())
    return P_CAST_FLOAT2FLOAT;

  return P_CAST_INVALID;
}

/* Classify bool -> p_to_type cast. */
static PAstCastKind
classify_bool_cast(PType* p_to_type)
{
  if (p_to_type->is_int_ty())
    return P_CAST_BOOL2INT;

  if (p_to_type->is_float_ty())
    return P_CAST_BOOL2FLOAT;

  return P_CAST_INVALID;
}

PAstCastExpr*
PSema::act_on_cast_expr(PAstExpr* p_sub_expr, PType* p_target_ty, PSourceRange p_src_range, PSourceLocation p_as_loc)
{
  assert(p_sub_expr != nullptr && p_target_ty != nullptr);

  PType* from_type = p_sub_expr->get_type(m_context);

  PAstCastKind cast_kind = P_CAST_INVALID;
  if (from_type->get_canonical_ty() == p_target_ty->get_canonical_ty())
    cast_kind = P_CAST_NOOP; // from/to types are equivalent
  else if (from_type->is_int_ty())
    cast_kind = classify_int_cast(from_type, p_target_ty);
  else if (from_type->is_float_ty())
    cast_kind = classify_float_cast(p_target_ty);
  else if (from_type->is_bool_ty())
    cast_kind = classify_bool_cast(p_target_ty);

  // TODO: issue a warning when cast is noop

  if (cast_kind == P_CAST_INVALID) {
    PDiag* d = diag_at(P_DK_err_unsupported_conversion, p_as_loc);
    diag_add_arg_type(d, from_type);
    diag_add_arg_type(d, p_target_ty);
    diag_flush(d);
    return nullptr;
  }

  auto* node = m_context.new_object<PAstCastExpr>(p_sub_expr, p_target_ty, cast_kind, p_src_range);
  return node;
}

PVarDecl*
PSema::act_on_var_decl(PType* p_type,
                       PIdentifierInfo* p_name,
                       PAstExpr* p_init_expr,
                       PSourceRange p_src_range,
                       PSourceRange p_name_range)
{
  // Unnamed variables are forbidden.
  if (p_name == nullptr) {
    PDiag* d = diag_at(P_DK_err_var_unnamed, p_src_range.begin);
    diag_add_source_caret(d, p_src_range.begin);
    diag_flush(d);
    return m_context.new_object<PVarDecl>(p_type, p_name, p_init_expr);
  }

  // Check for variable redeclaration.
  PSymbol* symbol = local_lookup(p_name);
  if (symbol != nullptr) {
    PDiag* d = diag_at(P_DK_err_var_redeclaration, p_name_range.begin);
    diag_add_arg_ident(d, p_name);
    diag_add_source_range(d, p_name_range);
    diag_flush(d);
    return m_context.new_object<PVarDecl>(p_type, p_name, p_init_expr);
  }

  // Deduce the variable type if needed.
  if (p_type == nullptr) {
    if (p_init_expr == nullptr) {
      PDiag* d = diag_at(P_DK_err_cannot_deduce_var_type, p_name_range.begin);
      diag_add_arg_ident(d, p_name);
      diag_add_source_range(d, p_name_range);
      diag_flush(d);
      return nullptr;
    }

    p_type = p_init_expr->get_type(m_context);
  } else if (p_init_expr != nullptr) {
    PType* init_expr_type = p_init_expr->get_type(m_context);
    if (!are_types_compatible(init_expr_type, p_type)) {
      PDiag* d = diag_at(P_DK_err_expected_type, p_init_expr->get_source_range().begin);
      diag_add_arg_type(d, p_type);
      diag_add_arg_type(d, init_expr_type);
      diag_add_source_range(d, p_init_expr->get_source_range());
      diag_flush(d);
      return nullptr;
    }
  }

  auto* node = m_context.new_object<PVarDecl>(p_type, p_name, p_init_expr);
  symbol = p_scope_add_symbol(m_current_scope, p_name);
  symbol->decl = node;
  return node;
}

PParamDecl*
PSema::act_on_param_decl(PType* p_type, PIdentifierInfo* p_name, PSourceRange p_src_range, PSourceRange p_name_range)
{
  // Unnamed parameters are forbidden.
  if (p_name == nullptr) {
    PDiag* d = diag_at(P_DK_err_param_unnamed, p_src_range.begin);
    diag_add_source_caret(d, p_src_range.begin);
    diag_flush(d);
    return m_context.new_object<PParamDecl>(p_type, p_name);
  }

  PSymbol* symbol = local_lookup(p_name);
  if (symbol != nullptr) {
    PDiag* d = diag_at(P_DK_err_param_name_already_used, p_name_range.begin);
    diag_add_arg_ident(d, p_name);
    diag_add_source_range(d, p_name_range);
    diag_flush(d);
    return m_context.new_object<PParamDecl>(p_type, p_name);
  }

  if (p_type == nullptr) {
    PDiag* d = diag_at(P_DK_err_param_type_required, p_src_range.begin);
    diag_add_arg_ident(d, p_name);
    diag_add_source_range(d, p_name_range);
    diag_flush(d);
    return m_context.new_object<PParamDecl>(p_type, p_name);
  }

  auto* node = m_context.new_object<PParamDecl>(p_type, p_name);
  symbol = p_scope_add_symbol(m_current_scope, p_name);
  symbol->decl = node;
  return node;
}

void
PSema::begin_func_decl_analysis(PFunctionDecl* p_decl)
{
  push_scope(P_SF_FUNC_PARAMS);

  m_curr_func_type = p_decl->type->as<PFunctionType>();

  // Register parameters on the current scope.
  // All parameters have already been checked.
  for (size_t i = 0; i < p_decl->param_count; ++i) {
    // We are tolerant for nullptrs to try recover errors during parsing.
    if (p_decl->params[i] != nullptr && p_decl->params[i]->name != nullptr) {
      PSymbol* symbol = p_scope_add_symbol(m_current_scope, p_decl->params[i]->name);
      symbol->decl = p_decl->params[i];
    }
  }
}

void
PSema::end_func_decl_analysis()
{
  pop_scope();
}

PFunctionDecl*
PSema::act_on_func_decl(PIdentifierInfo* p_name,
                        PType* p_ret_ty,
                        PParamDecl** p_params,
                        size_t p_param_count,
                        PSourceRange p_name_range,
                        PSourceLocation p_key_fn_end_loc)
{
  // Unnamed functions are forbidden.
  if (p_name == nullptr) {
    PDiag* d = diag_at(P_DK_err_func_unnamed, p_key_fn_end_loc);
    diag_flush(d);
  }

  PSymbol* symbol = local_lookup(p_name);
  if (symbol != nullptr) {
    PDiag* d = diag_at(P_DK_err_func_redeclaration, p_name_range.begin);
    diag_add_arg_ident(d, p_name);
    diag_add_source_range(d, p_name_range);
    diag_flush(d);
    return nullptr;
  }

  if (p_ret_ty == nullptr)
    p_ret_ty = m_context.get_void_ty();

  std::vector<PType*> param_tys(p_param_count);
  for (size_t i = 0; i < p_param_count; ++i) {
    param_tys[i] = p_params[i]->type;
  }

  auto* func_ty = m_context.get_function_ty(p_ret_ty, param_tys);

  auto* raw_params = m_context.alloc_object<PParamDecl*>(p_param_count);
  std::copy(p_params, p_params + p_param_count, raw_params);

  auto* decl = m_context.new_object<PFunctionDecl>(func_ty, p_name, raw_params, p_param_count);

  symbol = p_scope_add_symbol(m_current_scope, p_name);
  symbol->decl = decl;

  return decl;
}

PFunctionDecl*
PSema::act_on_func_decl(PIdentifierInfo* p_name,
                        PType* p_ret_ty,
                        const std::vector<PParamDecl*>& p_params,
                        PSourceRange p_name_range,
                        PSourceLocation p_key_fn_end_loc)
{
  return act_on_func_decl(
    p_name, p_ret_ty, const_cast<PParamDecl**>(p_params.data()), p_params.size(), p_name_range, p_key_fn_end_loc);
}

PAstExpr*
PSema::convert_to_rvalue(PAstExpr* p_expr)
{
  if (p_expr->is_rvalue())
    return p_expr;

  return m_context.new_object<PAstL2RValueExpr>(p_expr);
}

PDecl*
PSema::try_get_ref_decl(PAstExpr* p_expr)
{
  p_expr = p_expr->ignore_parens();
  if (p_expr->get_kind() == P_AST_NODE_DECL_REF_EXPR)
    return p_expr->as<PAstDeclRefExpr>()->decl;
  return nullptr;
}
