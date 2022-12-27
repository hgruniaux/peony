#pragma once

#include <hedley.h>

#include <cstddef>
#include <cstring>
#include <type_traits>

enum PTypeKind
{
#define TYPE(p_kind) p_kind,
#include "type.def"
};

class PType
{
public:
  void* m_llvm_cached_type;

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

  /// Returns `true` if this is canonically the `void` type.
  [[nodiscard]] bool is_void_ty() const { return get_canonical_kind() == P_TK_VOID; }
  /// Returns `true` if this is canonically the `bool` type.
  [[nodiscard]] bool is_bool_ty() const { return get_canonical_kind() == P_TK_BOOL; }
  /// Returns `true` if this is canonically a pointer type.
  [[nodiscard]] bool is_pointer_ty() const { return get_canonical_kind() == P_TK_POINTER; }
  /// Returns `true` if this is canonically a function type.
  [[nodiscard]] bool is_function_ty() const { return get_canonical_kind() == P_TK_FUNCTION; }

  /// Returns `true` if this is canonically a signed integer type (one of the `i*` types).
  [[nodiscard]] bool is_signed_int_ty() const;
  /// Returns `true` if this is canonically an unsigned integer type (one of the `u*` types).
  [[nodiscard]] bool is_unsigned_int_ty() const;
  [[nodiscard]] bool is_int_ty() const { return is_signed_int_ty() || is_unsigned_int_ty(); }
  [[nodiscard]] bool is_float_ty() const { return is_signed_int_ty() || is_unsigned_int_ty(); }
  /// Returns `true` if this is canonically an arithmetic type (either integer or float).
  [[nodiscard]] bool is_arithmetic_ty() const { return is_int_ty() || is_float_ty(); }

protected:
  friend class PContext;
  explicit PType(PTypeKind p_kind)
    : m_kind(p_kind)
    , m_canonical_type(this)
    , m_llvm_cached_type(nullptr)
  {
  }

  PTypeKind m_kind;
  PType* m_canonical_type;
};

/// A parenthesized type (e.g. `i32`).
/// Unlike other types, parenthesized types are unique and therefore pointer
/// comparison may not be appropriate.
class PParenType : public PType
{
public:
  [[nodiscard]] PType* get_sub_type() const { return m_sub_type; }

private:
  friend class PContext;
  explicit PParenType(PType* p_sub_type)
    : PType(P_TK_PAREN)
    , m_sub_type(p_sub_type)
  {
    m_canonical_type = p_sub_type->get_canonical_ty();
  }

  PType* m_sub_type;
};

class PFunctionType : public PType
{
public:
  /// Gets the return type (always non null).
  [[nodiscard]] PType* get_ret_ty() const { return m_ret_ty; }

  [[nodiscard]] size_t get_param_count() const { return m_param_count; }
  [[nodiscard]] PType** get_params() const { return const_cast<PType**>(m_params); }
  [[nodiscard]] PType** get_param_begin() const { return const_cast<PType**>(m_params); }
  [[nodiscard]] PType** get_param_end() const { return const_cast<PType**>(m_params + m_param_count); }

private:
  friend class PContext;
  PFunctionType(PType* p_ret_ty, PType** p_params_ty, size_t p_param_count)
    : PType(P_TK_FUNCTION)
    , m_ret_ty(p_ret_ty)
    , m_params(p_params_ty)
    , m_param_count(p_param_count)
  {
  }

  PType* m_ret_ty;
  PType** m_params;
  size_t m_param_count;
};

class PPointerType : public PType
{
public:
  [[nodiscard]] PType* get_element_ty() const { return m_element_ty; }

private:
  friend class PContext;
  PPointerType(PType* p_elt_ty)
    : PType(P_TK_POINTER)
    , m_element_ty(p_elt_ty)
  {
  }

  PType* m_element_ty;
};

class PArrayType : public PType
{
public:
  [[nodiscard]] PType* get_element_ty() const { return m_element_ty; }
  [[nodiscard]] size_t get_num_elements() const { return m_num_elements; }

private:
  friend class PContext;
  PArrayType(PType* p_elt_ty, size_t p_num_elt)
    : PType(P_TK_ARRAY)
    , m_element_ty(p_elt_ty)
    , m_num_elements(p_num_elt)
  {
  }

  size_t m_num_elements;
  PType* m_element_ty;
};

int
p_type_get_bitwidth(PType* p_type);

PType*
p_type_get_bool();
