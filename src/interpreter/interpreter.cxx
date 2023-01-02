#include "interpreter.hxx"

PInterpreter::PInterpreter(PContext& p_ctx)
  : m_ctx(p_ctx)
{
}

PInterpreterValue
PInterpreter::eval(const PAstExpr* p_expr)
{
  assert(m_value_stack.empty());

  if (p_expr == nullptr)
    return PInterpreterValue::make_indeterminate();

  visit(p_expr);
  assert(m_value_stack.size() == 1);
  return pop_value();
}

std::optional<bool>
PInterpreter::eval_as_bool(const PAstExpr* p_expr)
{
  auto value = eval(p_expr);
  if (value.is_bool())
    return value.get_bool();
  return std::nullopt;
}

std::optional<intmax_t>
PInterpreter::eval_as_int(const PAstExpr* p_expr)
{
  auto value = eval(p_expr);
  if (value.is_integer())
    return value.get_integer();
  return std::nullopt;
}

std::optional<double>
PInterpreter::eval_as_float(const PAstExpr* p_expr)
{
  auto value = eval(p_expr);
  if (value.is_float())
    return value.get_float();
  return std::nullopt;
}

void PInterpreter::visit_expr(const PAstExpr* p_node)
{
  push_value(PInterpreterValue::make_indeterminate());
}

void
PInterpreter::visit_bool_literal(const PAstBoolLiteral* p_node)
{
  push_value(PInterpreterValue::make_bool(p_node->value));
}

void
PInterpreter::visit_int_literal(const PAstIntLiteral* p_node)
{
  push_value(PInterpreterValue::make_integer(p_node->value));
}

void
PInterpreter::visit_float_literal(const PAstFloatLiteral* p_node)
{
  push_value(PInterpreterValue::make_float(p_node->value));
}

void
PInterpreter::visit_paren_expr(const PAstParenExpr* p_node)
{
  visit(p_node->sub_expr);
}

void
PInterpreter::visit_unary_expr(const PAstUnaryExpr* p_node)
{
  visit(p_node->sub_expr);

  PInterpreterValue value = pop_value();
  switch (p_node->opcode) {
    case P_UNARY_NEG:
      push_value(-value);
      break;
    case P_UNARY_NOT:
      if (p_node->get_type(m_ctx)->is_int_ty())
        // FIXME: Implement bitwise not for integers. In order to do this, we need the bitwidth of the integer. Maybe
        // use
        //        the sub_expr type.
        push_value(PInterpreterValue::make_indeterminate());
      else
        push_value(!value);
      break;
    default:
      push_value(PInterpreterValue::make_indeterminate());
      break;
  }
}

void
PInterpreter::visit_binary_expr(const PAstBinaryExpr* p_node)
{
  // Special path for binary operators '&&' and '||' because they are lazy
  // operators.
  if (p_node->opcode == P_BINARY_LOG_AND || p_node->opcode == P_BINARY_LOG_OR) {
    visit(p_node->lhs);
    PInterpreterValue lhs = pop_value();

    if (!lhs.is_bool()) {
      push_value(PInterpreterValue::make_indeterminate());
      return;
    }

    bool lhs_value = lhs.get_bool();
    if (p_node->opcode == P_BINARY_LOG_AND) {
      if (lhs_value)
        visit(p_node->rhs);
      else
        push_value(PInterpreterValue::make_bool(false));
    } else { // p_node->opcode == P_BINARY_LOG_OR
      if (!lhs_value)
        visit(p_node->rhs);
      else
        push_value(PInterpreterValue::make_bool(true));
    }

    return;
  }

  visit(p_node->lhs);
  visit(p_node->rhs);

  PInterpreterValue rhs = pop_value();
  PInterpreterValue lhs = pop_value();
  switch (p_node->opcode) {
    case P_BINARY_ADD:
      lhs += rhs;
      break;
    case P_BINARY_SUB:
      lhs -= rhs;
      break;
    case P_BINARY_MUL:
      lhs *= rhs;
      break;
    case P_BINARY_DIV:
      lhs /= rhs;
      break;
    case P_BINARY_MOD:
      lhs %= rhs;
      break;
    case P_BINARY_SHL:
      lhs <<= rhs;
      break;
    case P_BINARY_SHR:
      lhs >>= rhs;
      break;
    case P_BINARY_BIT_AND:
      lhs &= rhs;
      break;
    case P_BINARY_BIT_OR:
      lhs |= rhs;
      break;
    case P_BINARY_BIT_XOR:
      lhs ^= rhs;
      break;
    default:
      lhs = PInterpreterValue::make_indeterminate();
      break;
  }

  push_value(std::move(lhs));
}

void
PInterpreter::visit_cast_expr(const PAstCastExpr* p_node)
{
  visit(p_node->sub_expr);
  PInterpreterValue value = pop_value();
  switch (p_node->cast_kind) {
    case P_CAST_NOOP:
    case P_CAST_INT2INT:
    case P_CAST_FLOAT2FLOAT:
      break;
    case P_CAST_BOOL2INT:
    case P_CAST_FLOAT2INT:
      value.into_integer();
      break;
    case P_CAST_BOOL2FLOAT:
    case P_CAST_INT2FLOAT:
      value.into_float();
      break;
    default:
      value.set_indeterminate();
      break;
  }

  push_value(std::move(value));
}

void
PInterpreter::push_value(const PInterpreterValue& p_value)
{
  m_value_stack.push(p_value);
}

void
PInterpreter::push_value(PInterpreterValue&& p_value)
{
  m_value_stack.push(std::move(p_value));
}

PInterpreterValue
PInterpreter::pop_value()
{
  auto value = m_value_stack.top();
  m_value_stack.pop();
  return value;
}
