#ifndef PEONY_AST_DECL_HXX
#define PEONY_AST_DECL_HXX

#include "context.hxx"
#include "identifier_table.hxx"
#include "type.hxx"
#include "utils/source_location.hxx"

class PAst;
class PAstExpr;

/* --------------------------------------------------------
 * Declarations
 */

enum PDeclKind
{
  P_DK_VAR,
  P_DK_PARAM,
  P_DK_FUNCTION,
  P_DK_STRUCT_FIELD,
  P_DK_STRUCT
};

/// \brief Base class for all declarations.
///
/// \see PAst
class PDecl
{
public:
  PDeclKind kind;
  PSourceRange source_range;
  // Some declarations may be unnamed, in that case this is nullptr.
  PIdentifierInfo* name;
  PType* type;
  // Is there at least one reference to this type?
  bool used;

  [[nodiscard]] PDeclKind get_kind() const { return kind; }

  template<class T>
  [[nodiscard]] T* as()
  {
    return static_cast<T*>(this);
  }
  template<class T>
  [[nodiscard]] const T* as() const
  {
    return const_cast<PDecl*>(this)->as<T>();
  }

  void dump(PContext& p_ctx);

protected:
  PDecl(PDeclKind p_kind, PType* p_type, PIdentifierInfo* p_name, PSourceRange p_src_range)
    : kind(p_kind)
    , source_range(p_src_range)
    , type(p_type)
    , name(p_name)
    , used(false)
  {
  }
};

/// \brief A variable declaration.
class PVarDecl : public PDecl
{
public:
  static constexpr auto DECL_KIND = P_DK_VAR;

  PAstExpr* init_expr;

  PVarDecl(PType* p_type, PIdentifierInfo* p_name, PAstExpr* p_init_expr = nullptr, PSourceRange p_src_range = {})
    : PDecl(DECL_KIND, p_type, p_name, p_src_range)
    , init_expr(p_init_expr)
  {
  }
};

/// \brief A function parameter declaration.
class PParamDecl : public PDecl
{
public:
  static constexpr auto DECL_KIND = P_DK_PARAM;

  PParamDecl(PType* p_type, PIdentifierInfo* p_name, PSourceRange p_src_range = {})
    : PDecl(DECL_KIND, p_type, p_name, p_src_range)
  {
  }
};

/// \brief A function declaration.
class PFunctionDecl : public PDecl
{
public:
  static constexpr auto DECL_KIND = P_DK_FUNCTION;

  PAst* body;
  PParamDecl** params;
  size_t param_count;

  PFunctionDecl(PType* p_type,
                PIdentifierInfo* p_name,
                PParamDecl** p_params,
                size_t p_param_count,
                PAst* p_body = nullptr,
                PSourceRange p_src_range = {})
    : PDecl(DECL_KIND, p_type, p_name, p_src_range)
    , body(nullptr)
    , params(p_params)
    , param_count(p_param_count)
  {
  }
};

class PStructDecl;

/// \brief A structure field declaration.
class PStructFieldDecl : public PDecl
{
public:
  static constexpr auto DECL_KIND = P_DK_STRUCT_FIELD;

  // To whom does this field belong.
  PStructDecl* parent;
  // This is the position in parent->fields where this field belong in.
  int idx_in_parent_fields;
};

/// \brief A structure declaration.
class PStructDecl : public PDecl
{
public:
  static constexpr auto DECL_KIND = P_DK_STRUCT;

  size_t field_count;
  PStructFieldDecl* fields[1]; // tail-allocated
};

#endif // PEONY_AST_DECL_HXX
