#include "sema.hxx"

#include "parser.hxx"
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
    if (symbol != nullptr) {
      assert(symbol->decl != nullptr);
      return symbol;
    }

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

PType*
PSema::lookup_type(PLocalizedIdentifierInfo p_name, PDiagKind p_diag, bool p_return_unknown) const
{
  PSymbol* symbol = lookup(p_name.ident);
  if (symbol != nullptr && symbol->decl->get_kind() == P_DK_STRUCT)
    return symbol->decl->get_type();

  PDiag* d = diag_at(p_diag, p_name.range.begin);
  diag_add_source_range(d, p_name.range);
  diag_add_arg_ident(d, p_name.ident);
  diag_flush(d);

  if (p_return_unknown) {
    // Return a fake type which store the name as typed by the user.
    // AST consumers can then use it for diagnostics, resilient formatting, etc.
    return m_context.get_unknown_ty(p_name.ident);
  } else {
    return nullptr;
  }
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
      assert(false && "unreachable");
      return nullptr;
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
      assert(false && "unreachable");
      return nullptr;
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
PSema::act_on_let_stmt(PArrayView<PVarDecl*> p_decls, PSourceRange p_src_range)
{
  return m_context.new_object<PAstLetStmt>(make_array_view_copy(p_decls), p_src_range);
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
PSema::act_on_return_stmt(PAstExpr* p_ret_expr, PSourceRange p_src_range)
{
  // Check if the return expression type and the current function return type are compatible
  // and if not then emit a diagnostic.
  PType* type = (p_ret_expr != nullptr) ? p_ret_expr->get_type(m_context) : m_context.get_void_ty();
  if (!is_compatible_with_ret_ty(type)) {
    // The location of the ';'.
    PSourceLocation semi_loc = p_src_range.end - 1;
    PSourceLocation diag_loc = (p_ret_expr != nullptr) ? p_ret_expr->get_source_range().begin : semi_loc;

    PDiag* d = diag_at(P_DK_err_expected_type, diag_loc);
    diag_add_arg_type(d, m_curr_func_type->get_ret_ty());
    diag_add_arg_type(d, type);

    if (p_ret_expr != nullptr)
      diag_add_source_range(d, p_ret_expr->get_source_range());

    diag_flush(d);
  }

  if (p_ret_expr != nullptr)
    p_ret_expr = convert_to_rvalue(p_ret_expr);
  return m_context.new_object<PAstReturnStmt>(p_ret_expr, p_src_range);
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
PSema::act_on_unary_expr(PAstExpr* p_sub_expr, PAstUnaryOp p_opcode, PSourceRange p_src_range)
{
  assert(p_sub_expr != nullptr);

  // The location of the unary operator.
  const PSourceLocation op_loc = p_src_range.begin;
  PType* result_type = p_sub_expr->get_type(m_context);
  switch (p_opcode) {
    case P_UNARY_NEG: // '-' operator
      if (!result_type->is_float_ty() && !result_type->is_signed_int_ty()) {
        PDiag* d = diag_at(P_DK_err_cannot_apply_unary_op, op_loc);
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
        PDiag* d = diag_at(P_DK_err_cannot_apply_unary_op, op_loc);
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
        PDiag* d = diag_at(P_DK_err_could_not_take_addr_rvalue, op_loc);
        diag_add_arg_type(d, result_type);
        diag_add_source_range(d, p_sub_expr->get_source_range());
        diag_flush(d);
        return nullptr;
      }

      result_type = m_context.get_pointer_ty(result_type);
      break;
    case P_UNARY_DEREF: // '*' operator
      if (!result_type->is_pointer_ty()) {
        PDiag* d = diag_at(P_DK_err_indirection_requires_ptr, op_loc);
        diag_add_arg_type(d, result_type);
        diag_add_source_range(d, p_sub_expr->get_source_range());
        diag_flush(d);
        return nullptr;
      }

      result_type = result_type->as<PPointerType>()->get_element_ty();
      break;
    default:
      assert(false && "unknown unary operator");
      return nullptr;
  }

  return m_context.new_object<PAstUnaryExpr>(p_sub_expr, result_type, p_opcode, p_src_range);
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
                       PArrayView<PAstExpr*> p_args,
                       PFunctionDecl* p_func_decl,
                       PSourceLocation p_lparen_loc)
{
  const size_t required_param_count = p_callee_ty->get_param_count();
  if (p_args.size() != required_param_count) {
    PDiag* d =
      diag_at((p_args.size() < required_param_count) ? P_DK_err_too_few_args : P_DK_err_too_many_args, p_lparen_loc);
    if (p_func_decl != nullptr)
      diag_add_arg_type_with_name_hint(d, p_callee_ty, p_func_decl->get_name());
    else
      diag_add_arg_type(d, p_callee_ty);
    diag_add_source_caret(d, p_lparen_loc);
    diag_flush(d);
    return;
  }

  PArrayView<PType*> args_expected_type = p_callee_ty->get_params();
  for (size_t i = 0; i < p_args.size(); ++i) {
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

PAstMemberExpr*
PSema::act_on_member_expr(PAstExpr* p_base_expr,
                          PLocalizedIdentifierInfo p_member_name,
                          PSourceRange p_src_range,
                          PSourceLocation p_dot_loc)
{
  PType* base_type = p_base_expr->get_type(m_context);
  PDecl* base_decl = base_type->is_tag_ty() ? base_type->get_canonical_ty()->as<PTagType>()->get_decl() : nullptr;
  if (base_decl == nullptr || base_decl->get_kind() != P_DK_STRUCT) {
    PDiag* d = diag_at(P_DK_err_member_not_struct, p_base_expr->get_source_range().begin);
    diag_add_source_range(d, p_base_expr->get_source_range());
    diag_add_arg_ident(d, p_member_name.ident);
    diag_flush(d);
    return nullptr;
  }

  auto* field = base_decl->as<PStructDecl>()->find_field(p_member_name.ident);
  if (field == nullptr) {
    PDiag* d = diag_at(P_DK_err_no_member_named, p_member_name.range.begin);
    diag_add_source_range(d, p_member_name.range);
    diag_add_arg_type(d, base_type);
    diag_add_arg_ident(d, p_member_name.ident);
    diag_flush(d);
    return nullptr;
  }

  return m_context.new_object<PAstMemberExpr>(p_base_expr, field, p_src_range);
}

PAstCallExpr*
PSema::act_on_call_expr(PAstExpr* p_callee,
                        PArrayView<PAstExpr*> p_args,
                        PSourceRange p_src_range,
                        PSourceLocation p_lparen_loc)
{
  assert(p_callee != nullptr);

  auto* callee_decl = try_get_ref_decl(p_callee);
  auto* callee_ty = p_callee->get_type(m_context);
  if (!callee_ty->is_function_ty()) {
    PDiag* d;

    if (callee_decl != nullptr) {
      d = diag_at(P_DK_err_cannot_be_used_as_function, p_lparen_loc);
      diag_add_arg_ident(d, callee_decl->get_name());
    } else {
      d = diag_at(P_DK_err_expr_cannot_be_used_as_function, p_lparen_loc);
    }

    diag_add_source_range(d, p_callee->get_source_range());
    diag_add_source_caret(d, p_lparen_loc);
    diag_flush(d);
    return nullptr;
  }

  check_call_args(callee_ty->as<PFunctionType>(), p_args, (PFunctionDecl*)callee_decl, p_lparen_loc);
  auto* node = m_context.new_object<PAstCallExpr>(p_callee, make_array_view_copy(p_args), p_src_range);

  // Convert all lvalue arguments to rvalue
  for (size_t i = 0; i < p_args.size(); ++i) {
    node->args[i] = convert_to_rvalue(node->args[i]);
  }

  return node;
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

PStructDecl*
PSema::resolve_struct_expr_name(PLocalizedIdentifierInfo p_name)
{
  assert(p_name.ident != nullptr);

  auto* referenced_type = lookup_type(p_name, P_DK_err_cannot_find_struct, false);
  if (referenced_type == nullptr) {
    return nullptr;
  } else if (!referenced_type->is_tag_ty() ||
             referenced_type->as_canonical<PTagType>()->get_decl()->get_kind() != P_DK_STRUCT) {
    PDiag* d = diag_at(P_DK_err_expected_struct, p_name.range.begin);
    diag_add_source_range(d, p_name.range);
    diag_add_arg_ident(d, p_name.ident);
    diag_add_arg_type(d, referenced_type);
    diag_flush(d);
    return nullptr;
  }

  return referenced_type->as_canonical<PTagType>()->get_decl()->as<PStructDecl>();
}

PAstStructFieldExpr*
PSema::act_on_struct_field_expr(PStructDecl* p_struct_decl, PLocalizedIdentifierInfo p_name, PAstExpr* p_expr)
{
  assert(p_struct_decl != nullptr && p_name.ident != nullptr);

  auto* field = p_struct_decl->find_field(p_name.ident);
  if (field == nullptr) {
    PDiag* d = diag_at(P_DK_err_struct_has_not_field, p_name.range.begin);
    diag_add_source_range(d, p_name.range);
    diag_add_arg_ident(d, p_struct_decl->get_name());
    diag_add_arg_ident(d, p_name.ident);
    diag_flush(d);
    return nullptr;
  }

  // Is a shorthand struct field expr? That is a field expression with only a name like 'a' in
  // MyStruct { a }
  bool is_shorthand = (p_expr == nullptr);
  if (is_shorthand) {
    auto* decl_ref = act_on_decl_ref_expr(p_name.ident, p_name.range);
    p_expr = convert_to_rvalue(decl_ref);
  }

  auto* expr_ty = p_expr->get_type(m_context);
  if (!are_types_compatible(expr_ty, field->get_type())) {
    PDiag* d = diag_at(P_DK_err_expected_type, p_expr->get_source_range().begin);
    diag_add_source_range(d, p_expr->get_source_range());
    diag_add_arg_type(d, field->get_type());
    diag_add_arg_type(d, expr_ty);
    diag_flush(d);
  }

  return m_context.new_object<PAstStructFieldExpr>(field, p_expr, is_shorthand);
}

PAstStructExpr*
PSema::act_on_struct_expr(PStructDecl* p_struct_decl,
                          PArrayView<PAstStructFieldExpr*> p_fields,
                          PSourceRange p_src_range)
{
  assert(p_struct_decl != nullptr);
  return m_context.new_object<PAstStructExpr>(p_struct_decl, make_array_view_copy(p_fields), p_src_range);
}

PAstTranslationUnit*
PSema::act_on_translation_unit(PArrayView<PDecl*> p_decls)
{
  return m_context.new_object<PAstTranslationUnit>(make_array_view_copy(p_decls));
}

PVarDecl*
PSema::act_on_var_decl(PType* p_type, PLocalizedIdentifierInfo p_name, PAstExpr* p_init_expr, PSourceRange p_src_range)
{
  // Unnamed variables are forbidden.
  if (p_name.ident == nullptr) {
    PDiag* d = diag_at(P_DK_err_var_unnamed, p_src_range.begin);
    diag_add_source_caret(d, p_src_range.begin);
    diag_flush(d);
    return m_context.new_object<PVarDecl>(p_type, p_name, p_init_expr);
  }

  // Check for variable redeclaration.
  PSymbol* symbol = local_lookup(p_name.ident);
  if (symbol != nullptr) {
    PDiag* d = diag_at(P_DK_err_var_redeclaration, p_name.range.begin);
    diag_add_arg_ident(d, p_name.ident);
    diag_add_source_range(d, p_name.range);
    diag_flush(d);
  }

  // Deduce the variable type if needed.
  if (p_type == nullptr) {
    if (p_init_expr == nullptr) {
      PDiag* d = diag_at(P_DK_err_cannot_deduce_var_type, p_name.range.begin);
      diag_add_arg_ident(d, p_name.ident);
      diag_add_source_range(d, p_name.range);
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

  auto* node = m_context.new_object<PVarDecl>(p_type, p_name, p_init_expr, p_src_range);

  if (symbol == nullptr) {
    symbol = p_scope_add_symbol(m_current_scope, p_name.ident);
    symbol->decl = node;
  }

  return node;
}

PParamDecl*
PSema::act_on_param_decl(PType* p_type, PLocalizedIdentifierInfo p_name, PSourceRange p_src_range)
{
  // Unnamed parameters are forbidden.
  if (p_name.ident == nullptr) {
    PDiag* d = diag_at(P_DK_err_param_unnamed, p_src_range.begin);
    diag_add_source_caret(d, p_src_range.begin);
    diag_flush(d);
    return m_context.new_object<PParamDecl>(p_type, p_name);
  }

  PSymbol* symbol = local_lookup(p_name.ident);
  if (symbol != nullptr) {
    PDiag* d = diag_at(P_DK_err_param_name_already_used, p_name.range.begin);
    diag_add_arg_ident(d, p_name.ident);
    diag_add_source_range(d, p_name.range);
    diag_flush(d);
  }

  if (p_type == nullptr) {
    PDiag* d = diag_at(P_DK_err_param_type_required, p_src_range.begin);
    diag_add_arg_ident(d, p_name.ident);
    diag_add_source_range(d, p_name.range);
    diag_flush(d);
    return m_context.new_object<PParamDecl>(p_type, p_name);
  }

  auto* node = m_context.new_object<PParamDecl>(p_type, p_name, p_src_range);

  if (symbol == nullptr) {
    symbol = p_scope_add_symbol(m_current_scope, p_name.ident);
    symbol->decl = node;
  }

  return node;
}

void
PSema::begin_func_decl_analysis(PFunctionDecl* p_decl)
{
  push_scope(P_SF_FUNC_PARAMS);

  m_curr_func_type = p_decl->get_type()->as<PFunctionType>();

  // Register parameters on the current scope.
  // All parameters have already been checked.
  for (auto param : p_decl->params) {
    // We are tolerant for nullptrs to try recover errors during parsing.
    if (param != nullptr && param->get_name() != nullptr) {
      PSymbol* symbol = p_scope_add_symbol(m_current_scope, param->get_name());
      symbol->decl = param;
    }
  }
}

void
PSema::end_func_decl_analysis()
{
  pop_scope();
}

PFunctionDecl*
PSema::act_on_func_decl(PLocalizedIdentifierInfo p_name,
                        PType* p_ret_ty,
                        PArrayView<PParamDecl*> p_params,
                        PSourceLocation p_lparen_loc)
{
  // Unnamed functions are forbidden.
  if (p_name.ident == nullptr) {
    PDiag* d = diag_at(P_DK_err_func_unnamed, p_lparen_loc);
    diag_add_source_caret(d, p_lparen_loc);
    diag_flush(d);
  }

  PSymbol* symbol = local_lookup(p_name.ident);
  if (symbol != nullptr) {
    PDiag* d = diag_at(P_DK_err_name_defined_multiple_times, p_name.range.begin);
    diag_add_arg_ident(d, p_name.ident);
    diag_add_source_range(d, p_name.range);
    diag_flush(d);
  }

  if (p_ret_ty == nullptr)
    p_ret_ty = m_context.get_void_ty();

  std::vector<PType*> param_tys(p_params.size());
  for (size_t i = 0; i < p_params.size(); ++i) {
    param_tys[i] = p_params[i]->get_type();
  }

  auto* func_ty = m_context.get_function_ty(p_ret_ty, param_tys);
  auto* decl = m_context.new_object<PFunctionDecl>(func_ty, p_name, make_array_view_copy(p_params));

  if (symbol == nullptr) {
    symbol = p_scope_add_symbol(m_current_scope, p_name.ident);
    symbol->decl = decl;
  }

  return decl;
}

PStructFieldDecl*
PSema::act_on_struct_field_decl(PType* p_type, PLocalizedIdentifierInfo p_name, PSourceRange p_src_range)
{
  return m_context.new_object<PStructFieldDecl>(p_type, p_name, p_src_range);
}

void
PSema::check_struct_fields(PArrayView<PStructFieldDecl*> p_fields)
{
  if (p_fields.empty())
    return; // nothing to check

  // Check for duplicate names in the struct fields.
  for (auto it = std::next(p_fields.begin()); it != p_fields.end(); ++it) {
    PIdentifierInfo* field_name = (*it)->get_name();
    bool has_duplicate = std::find_if(p_fields.begin(), it, [field_name](auto* p_other) {
                           return p_other->get_name() == field_name;
                         }) != it;
    if (has_duplicate) {
      PDiag* d = diag_at(P_DK_err_duplicate_member, (*it)->source_range.begin);
      diag_add_arg_ident(d, field_name);
      diag_add_source_range(d, (*it)->get_name_range());
      // TODO: Add a note diagnostic showing the previous declaration.
      diag_flush(d);
    }
  }
}

PStructDecl*
PSema::act_on_struct_decl(PLocalizedIdentifierInfo p_name,
                          PArrayView<PStructFieldDecl*> p_fields,
                          PSourceRange p_src_range)
{
  // Unnamed structures are forbidden.
  if (p_name.ident == nullptr) {
    PDiag* d = diag_at(P_DK_err_func_unnamed, p_name.range.begin);
    diag_add_source_caret(d, p_name.range.begin);
    diag_flush(d);
  }

  PSymbol* symbol = local_lookup(p_name.ident);
  if (symbol != nullptr) {
    PDiag* d = diag_at(P_DK_err_name_defined_multiple_times, p_name.range.begin);
    diag_add_arg_ident(d, p_name.ident);
    diag_add_source_range(d, p_name.range);
    // TODO: Add a note diagnostic showing the previous declaration.
    diag_flush(d);
  }

  check_struct_fields(p_fields);
  auto* decl = m_context.new_object<PStructDecl>(m_context, p_name, make_array_view_copy(p_fields), p_src_range);

  if (symbol == nullptr) {
    symbol = p_scope_add_symbol(m_current_scope, p_name.ident);
    symbol->decl = decl;
  }

  return decl;
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
  if (p_expr->get_kind() == P_SK_DECL_REF_EXPR)
    return p_expr->as<PAstDeclRefExpr>()->decl;
  return nullptr;
}
