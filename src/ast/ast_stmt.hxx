#ifndef PEONY_AST_STMT_HXX
#define PEONY_AST_STMT_HXX

#include "context.hxx"
#include "identifier_table.hxx"
#include "type.hxx"
#include "utils/source_location.hxx"
#include "utils/array_view.hxx"

#include <cassert>

class PDecl;
class PVarDecl;
class PAstExpr;

/* --------------------------------------------------------
 * Statements
 */

enum PStmtKind
{
  P_SK_TRANSLATION_UNIT,
  P_SK_COMPOUND_STMT,
  P_SK_LET_STMT,
  P_SK_BREAK_STMT,
  P_SK_CONTINUE_STMT,
  P_SK_RETURN_STMT,
  P_SK_LOOP_STMT,
  P_SK_WHILE_STMT,
  P_SK_IF_STMT,
  P_SK_BOOL_LITERAL,
  P_SK_INT_LITERAL,
  P_SK_FLOAT_LITERAL,
  P_SK_PAREN_EXPR,
  P_SK_DECL_REF_EXPR,
  P_SK_UNARY_EXPR,
  P_SK_BINARY_EXPR,
  P_SK_MEMBER_EXPR,
  P_SK_CALL_EXPR,
  P_SK_CAST_EXPR,
  P_SK_STRUCT_EXPR,
  P_SK_L2RVALUE_EXPR
};

/// \brief Base class for all statements (including expressions).
class PAst
{
public:
  [[nodiscard]] PStmtKind get_kind() const { return m_kind; }

  [[nodiscard]] PSourceRange get_source_range() const { return m_source_range; }
  void set_source_range(PSourceRange p_src_range) { m_source_range = p_src_range; }
  void set_source_range(PSourceLocation p_beg, PSourceLocation p_end) { m_source_range = { p_beg, p_end }; }

  template<class T>
  [[nodiscard]] T* as()
  {
    return static_cast<T*>(this);
  }
  template<class T>
  [[nodiscard]] const T* as() const
  {
    return const_cast<PAst*>(this)->as<T>();
  }

  void dump(PContext& p_ctx);

protected:
  PAst(PStmtKind p_kind, PSourceRange p_src_range)
    : m_kind(p_kind)
    , m_source_range(p_src_range)
  {
  }

private:
  PStmtKind m_kind;
  PSourceRange m_source_range;
};

class PAstTranslationUnit : public PAst
{
public:
  static constexpr auto STMT_KIND = P_SK_TRANSLATION_UNIT;

  PSourceFile* p_src_file;
  PArrayView<PDecl*> decls;

  PAstTranslationUnit(PArrayView<PDecl*> p_decls, PSourceRange p_src_range = {})
    : PAst(STMT_KIND, p_src_range)
    , decls(p_decls)
  {
  }
};

/// \brief A `{ stmt... }` statement.
class PAstCompoundStmt : public PAst
{
public:
  static constexpr auto STMT_KIND = P_SK_COMPOUND_STMT;

  PArrayView<PAst*> stmts;

  PAstCompoundStmt(PArrayView<PAst*> p_stmts, PSourceRange p_src_range = {})
    : PAst(STMT_KIND, p_src_range)
    , stmts(p_stmts)
  {
  }
};

/// \brief A `let name = init_expr;` statement.
class PAstLetStmt : public PAst
{
public:
  static constexpr auto STMT_KIND = P_SK_LET_STMT;

  PArrayView<PVarDecl*> var_decls;

  explicit PAstLetStmt(PArrayView<PVarDecl*> p_var_decls, PSourceRange p_src_range = {})
    : PAst(STMT_KIND, p_src_range)
    , var_decls(p_var_decls)
  {
  }
};

/// \brief A `break;` statement.
class PAstBreakStmt : public PAst
{
public:
  static constexpr auto STMT_KIND = P_SK_BREAK_STMT;

  explicit PAstBreakStmt(PSourceRange p_src_range = {})
    : PAst(STMT_KIND, p_src_range)
  {
  }
};

/// \brief A `continue;` statement.
class PAstContinueStmt : public PAst
{
public:
  static constexpr auto STMT_KIND = P_SK_CONTINUE_STMT;

  explicit PAstContinueStmt(PSourceRange p_src_range = {})
    : PAst(STMT_KIND, p_src_range)
  {
  }
};

/// \brief A `return ret_expr;` statement.
class PAstReturnStmt : public PAst
{
public:
  static constexpr auto STMT_KIND = P_SK_RETURN_STMT;

  PAstExpr* ret_expr;

  explicit PAstReturnStmt(PAstExpr* p_ret_expr, PSourceRange p_src_range = {})
    : PAst(STMT_KIND, p_src_range)
    , ret_expr(p_ret_expr)
  {
  }
};

/// \brief A `if cond_expr { then_stmt } else { else_stmt }` (else_stmt is optional).
class PAstIfStmt : public PAst
{
public:
  static constexpr auto STMT_KIND = P_SK_IF_STMT;

  PAstExpr* cond_expr;
  PAst* then_stmt;
  PAst* else_stmt;

  PAstIfStmt(PAstExpr* p_cond_expr, PAst* p_then_stmt, PAst* p_else_stmt, PSourceRange p_src_range = {})
    : PAst(STMT_KIND, p_src_range)
    , cond_expr(p_cond_expr)
    , then_stmt(p_then_stmt)
    , else_stmt(p_else_stmt)
  {
    assert(p_cond_expr != nullptr && p_then_stmt != nullptr);
  }
};

/// \brief A `loop { body_stmt }`.
class PAstLoopStmt : public PAst
{
public:
  static constexpr auto STMT_KIND = P_SK_LOOP_STMT;

  PAst* body_stmt;

  explicit PAstLoopStmt(PAst* p_body, PSourceRange p_src_range = {})
    : PAstLoopStmt(STMT_KIND, p_body, p_src_range)
  {
  }

protected:
  PAstLoopStmt(PStmtKind p_kind, PAst* p_body, PSourceRange p_src_range)
    : PAst(p_kind, p_src_range)
    , body_stmt(p_body)
  {
  }
};

/// \brief A `while cond_expr { body_stmt }`.
class PAstWhileStmt : public PAstLoopStmt
{
public:
  static constexpr auto STMT_KIND = P_SK_WHILE_STMT;

  PAstExpr* cond_expr;

  PAstWhileStmt(PAstExpr* p_cond_expr, PAst* p_body, PSourceRange p_src_range = {})
    : PAstLoopStmt(STMT_KIND, p_body, p_src_range)
    , cond_expr(p_cond_expr)
  {
  }
};

#endif // PEONY_AST_STMT_HXX
