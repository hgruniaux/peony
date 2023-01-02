#ifndef PEONY_INTERPRETER_HXX
#define PEONY_INTERPRETER_HXX

#include "../ast/ast_visitor.hxx"
#include "value.hxx"

#include <stack>
#include <optional>

class PInterpreter : public PAstConstVisitor<PInterpreter>
{
public:
  PInterpreter(PContext& p_ctx);

  PInterpreterValue eval(const PAstExpr* p_expr);

  /// Like eval() but expects the result to be a boolean.
  /// If the result is not a boolean or is indeterminate, then std::std::nullopt is returned.
  std::optional<bool> eval_as_bool(const PAstExpr* p_expr);
  /// Like eval() but expects the result to be a boolean.
  /// If the result is not a int or is indeterminate, then std::std::nullopt is returned.
  std::optional<intmax_t> eval_as_int(const PAstExpr* p_expr);
  /// Like eval() but expects the result to be a float.
  /// If the result is not a float or is indeterminate, then std::std::nullopt is returned.
  std::optional<double> eval_as_float(const PAstExpr* p_expr);

  void visit_expr(const PAstExpr* p_node);

  void visit_bool_literal(const PAstBoolLiteral* p_node);
  void visit_int_literal(const PAstIntLiteral* p_node);
  void visit_float_literal(const PAstFloatLiteral* p_node);
  void visit_paren_expr(const PAstParenExpr* p_node);
  void visit_unary_expr(const PAstUnaryExpr* p_node);
  void visit_binary_expr(const PAstBinaryExpr* p_node);
  void visit_cast_expr(const PAstCastExpr* p_node);

private:
  void push_value(const PInterpreterValue& p_value);
  void push_value(PInterpreterValue&& p_value);
  PInterpreterValue pop_value();

private:
  PContext& m_ctx;
  std::stack<PInterpreterValue> m_value_stack;
};

#endif // PEONY_INTERPRETER_HXX
