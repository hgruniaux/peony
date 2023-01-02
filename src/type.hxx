#ifndef PEONY_TYPE_HXX
#define PEONY_TYPE_HXX

#include "identifier_table.hxx"
#include "utils/array_view.hxx"

#include <cstddef>
#include <cstring>
#include <type_traits>

class PDecl;
class PContext;

enum PTypeKind
{
#define TYPE(p_kind) p_kind,
#include "type.def"
};

class PType
{
public:
  [[nodiscard]] PTypeKind get_kind() const { return m_kind; }
  /// Shorthand for `get_canonical_ty()->get_kind()`
  [[nodiscard]] PTypeKind get_canonical_kind() const { return get_canonical_ty()->m_kind; }

  /// Checks if the type it is a canonical type or not, that is if `get_canonical_ty() == this`.
  [[nodiscard]] bool is_canonical_ty() const { return m_canonical_type == this; }
  [[nodiscard]] PType* get_canonical_ty() const { return m_canonical_type; }

  template<class T>
  [[nodiscard]] T* as()
  {
    static_assert(std::is_base_of_v<PType, T>, "T must inherits from PType");
    return static_cast<T*>(this);
  }
  template<class T>
  [[nodiscard]] const T* as() const
  {
    return const_cast<PType*>(this)->as<T>();
  }

  template<class T>
  [[nodiscard]] T* as_canonical()
  {
    static_assert(std::is_base_of_v<PType, T>, "T must inherits from PType");
    return static_cast<T*>(get_canonical_ty());
  }
  template<class T>
  [[nodiscard]] const T* as_canonical() const
  {
    return const_cast<PType*>(this)->as_canonical<T>();
  }

  /// Returns `true` if this is canonically the `void` type.
  [[nodiscard]] bool is_void_ty() const { return get_canonical_kind() == P_TK_VOID; }
  /// Returns `true` if this is canonically the `bool` type.
  [[nodiscard]] bool is_bool_ty() const { return get_canonical_kind() == P_TK_BOOL; }
  /// Returns `true` if this is canonically a pointer type.
  [[nodiscard]] bool is_pointer_ty() const { return get_canonical_kind() == P_TK_POINTER; }
  /// Returns `true` if this is canonically a function type.
  [[nodiscard]] bool is_function_ty() const { return get_canonical_kind() == P_TK_FUNCTION; }
  /// Returns `true` if this is canonically a function type.
  [[nodiscard]] bool is_tag_ty() const { return get_canonical_kind() == P_TK_TAG; }

  /// Returns `true` if this is canonically a signed integer type (one of the `i*` types).
  [[nodiscard]] bool is_signed_int_ty() const;
  /// Returns `true` if this is canonically an unsigned integer type (one of the `u*` types).
  [[nodiscard]] bool is_unsigned_int_ty() const;
  [[nodiscard]] bool is_int_ty() const { return is_signed_int_ty() || is_unsigned_int_ty(); }
  [[nodiscard]] bool is_float_ty() const;
  /// Returns `true` if this is canonically an arithmetic type (either integer or float).
  [[nodiscard]] bool is_arithmetic_ty() const { return is_int_ty() || is_float_ty(); }

  /// Returns the equivalent signed integer type (e.g. for `u32` returns `i32`).
  /// If it is already a signed integer type, it returns itself. However, if it
  /// is not an integer type (nor signed nor unsigned) then an error occurs.
  [[nodiscard]] PType* to_signed_int_ty(PContext& p_ctx) const;
  /// Same as to_signed_int_ty() but returns the equivalent unsigned integer
  /// type (e.f. for `i32` returns `u32`).
  [[nodiscard]] PType* to_unsigned_int_ty(PContext& p_ctx) const;

protected:
  friend class PContext;
  explicit PType(PTypeKind p_kind)
    : m_kind(p_kind)
    , m_canonical_type(this)
  {
  }

  PTypeKind m_kind;
  PType* m_canonical_type;
};

/// A parenthesized type (e.g. `i32`).
/// Unlike other types, parenthesized types are not unique and therefore pointer
/// comparison may not be appropriate.
class PParenType : public PType
{
public:
  static constexpr auto TYPE_KIND = P_TK_PAREN;

  [[nodiscard]] PType* get_sub_type() const { return m_sub_type; }

private:
  friend class PContext;
  explicit PParenType(PType* p_sub_type)
    : PType(TYPE_KIND)
    , m_sub_type(p_sub_type)
  {
    m_canonical_type = p_sub_type->get_canonical_ty();
  }

  PType* m_sub_type;
};

class PFunctionType : public PType
{
public:
  static constexpr auto TYPE_KIND = P_TK_FUNCTION;

  /// Gets the return type (always non null).
  [[nodiscard]] PType* get_ret_ty() const { return m_ret_ty; }

  [[nodiscard]] size_t get_param_count() const { return m_params.size(); }
  [[nodiscard]] PArrayView<PType*> get_params() const { return m_params; }

private:
  friend class PContext;
  PFunctionType(PType* p_ret_ty, PArrayView<PType*> p_params)
    : PType(TYPE_KIND)
    , m_ret_ty(p_ret_ty)
    , m_params(p_params)
  {
  }

  PType* m_ret_ty;
  PArrayView<PType*> m_params;
};

class PPointerType : public PType
{
public:
  static constexpr auto TYPE_KIND = P_TK_POINTER;

  [[nodiscard]] PType* get_element_ty() const { return m_element_ty; }

private:
  friend class PContext;
  explicit PPointerType(PType* p_elt_ty)
    : PType(TYPE_KIND)
    , m_element_ty(p_elt_ty)
  {
  }

  PType* m_element_ty;
};

class PArrayType : public PType
{
public:
  static constexpr auto TYPE_KIND = P_TK_ARRAY;

  [[nodiscard]] PType* get_element_ty() const { return m_element_ty; }
  [[nodiscard]] size_t get_num_elements() const { return m_num_elements; }

private:
  friend class PContext;
  PArrayType(PType* p_elt_ty, size_t p_num_elt)
    : PType(TYPE_KIND)
    , m_element_ty(p_elt_ty)
    , m_num_elements(p_num_elt)
  {
  }

  size_t m_num_elements;
  PType* m_element_ty;
};

class PTagType : public PType
{
public:
  static constexpr auto TYPE_KIND = P_TK_TAG;

  [[nodiscard]] PDecl* get_decl() const { return m_decl; }

private:
  friend class PContext;
  explicit PTagType(PDecl* p_decl)
    : PType(TYPE_KIND)
    , m_decl(p_decl)
  {
  }

  PDecl* m_decl;
};

/// A named unknown type.
///
/// This is generated by the semantic analyzer when it can not resolve type
/// names. This allows to store the name as typed by the user even if it is
/// not correct.
///
/// Unlike other types, unknown types are not unique and therefore pointer
/// comparison may not be appropriate.
class PUnknownType : public PType
{
public:
  static constexpr auto TYPE_KIND = P_TK_UNKNOWN;

  [[nodiscard]] PIdentifierInfo* get_name() const { return m_name; }

private:
  friend class PContext;
  explicit PUnknownType(PIdentifierInfo* p_name)
    : PType(TYPE_KIND)
    , m_name(p_name)
  {
  }

  PIdentifierInfo* m_name;
};

#endif // PEONY_TYPE_HXX
