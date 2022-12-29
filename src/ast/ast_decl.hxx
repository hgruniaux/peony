#ifndef PEONY_AST_DECL_HXX
#define PEONY_AST_DECL_HXX

#include "context.hxx"
#include "identifier_table.hxx"
#include "type.hxx"
#include "utils/array_view.hxx"
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
  // Is there at least one reference to this type?
  bool used;

  [[nodiscard]] PDeclKind get_kind() const { return kind; }

  [[nodiscard]] PLocalizedIdentifierInfo get_localized_name() const { return m_localized_name; }
  [[nodiscard]] PIdentifierInfo* get_name() const { return m_localized_name.ident; }
  [[nodiscard]] PSourceRange get_name_range() const { return m_localized_name.range; }

  [[nodiscard]] PType* get_type() const { return m_type; }

  [[nodiscard]] bool is_used() const { return m_used; }
  void mark_as_used() { m_used = true; }

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
  PDecl(PDeclKind p_kind, PType* p_type, PLocalizedIdentifierInfo p_name, PSourceRange p_src_range)
    : kind(p_kind)
    , source_range(p_src_range)
    , m_type(p_type)
    , m_localized_name(p_name)
    , used(false)
  {
  }

  PType* m_type;
  PLocalizedIdentifierInfo m_localized_name;
  bool m_used;
};

/// \brief A variable declaration.
class PVarDecl : public PDecl
{
public:
  static constexpr auto DECL_KIND = P_DK_VAR;

  PAstExpr* init_expr;

  PVarDecl(PType* p_type,
           PLocalizedIdentifierInfo p_name,
           PAstExpr* p_init_expr = nullptr,
           PSourceRange p_src_range = {})
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

  PParamDecl(PType* p_type, PLocalizedIdentifierInfo p_name, PSourceRange p_src_range = {})
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
  PArrayView<PParamDecl*> params;

  PFunctionDecl(PType* p_type,
                PLocalizedIdentifierInfo p_name,
                PArrayView<PParamDecl*> p_params,
                PAst* p_body = nullptr,
                PSourceRange p_src_range = {})
    : PDecl(DECL_KIND, p_type, p_name, p_src_range)
    , body(nullptr)
    , params(p_params)
  {
  }

  [[nodiscard]] bool has_body() const { return body != nullptr; }
  [[nodiscard]] PAst* get_body() const { return body; }
};

class PStructDecl;

/// \brief A structure field declaration.
class PStructFieldDecl : public PDecl
{
public:
  static constexpr auto DECL_KIND = P_DK_STRUCT_FIELD;

  PStructFieldDecl(PType* p_type, PLocalizedIdentifierInfo p_localized_name, PSourceRange p_src_range = {});
  ~PStructFieldDecl() = delete;

  [[nodiscard]] PStructDecl* get_parent() const { return m_parent; }

  /// Index of this field in its parent get_fields() array which is also the
  /// field position as originally typed in code.
  [[nodiscard]] size_t get_index_in_parent_fields() const { return m_index_in_parent_fields; }

private:
  friend class PStructDecl;
  PStructDecl* m_parent;
  size_t m_index_in_parent_fields;
};

/// \brief A structure declaration.
class PStructDecl : public PDecl
{
public:
  static constexpr auto DECL_KIND = P_DK_STRUCT;

  PStructDecl(PContext& p_ctx,
              PLocalizedIdentifierInfo p_name,
              PArrayView<PStructFieldDecl*> p_fields,
              PSourceRange p_src_range = {});
  ~PStructDecl() = delete;

  [[nodiscard]] size_t get_field_count() const { return m_fields.size(); }
  [[nodiscard]] PArrayView<PStructFieldDecl*> get_fields() const { return m_fields; }

  [[nodiscard]] bool has_field(PIdentifierInfo* p_name) const { return find_field(p_name) != nullptr; }
  [[nodiscard]] PStructFieldDecl* find_field(PIdentifierInfo* p_name) const;

private:
  PArrayView<PStructFieldDecl*> m_fields;
};

#endif // PEONY_AST_DECL_HXX
