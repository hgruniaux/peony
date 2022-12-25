#pragma once

#include <hedley.h>

#include <cstddef>
#include <cstring>

// WARNING: If you change the order of the following enum, you
// must update the functions:
//     - append_type() in name_mangling.c
//     - format_arg_type() in utils/diag_formatter_extra.c
typedef enum PTypeKind
{
  P_TYPE_VOID, // 'void' type
  P_TYPE_CHAR, // An Unicode scalar value (32-bit integer)
  P_TYPE_BOOL,
  P_TYPE_I8,
  P_TYPE_I16,
  P_TYPE_I32,
  P_TYPE_I64,
  P_TYPE_U8,
  P_TYPE_U16,
  P_TYPE_U32,
  P_TYPE_U64,
  P_TYPE_F32,
  P_TYPE_F64,
  P_TYPE_GENERIC_INT,
  P_TYPE_GENERIC_FLOAT,
  P_TYPE_LAST_BUILTIN = P_TYPE_GENERIC_FLOAT,
  P_TYPE_PAREN,    // A parenthesized type (e.g. '(i32)') implemented by PParenType
  P_TYPE_FUNCTION, // A function type implemented by PFunctionType
  P_TYPE_POINTER,  // A pointer type implemented by PPointerType
  P_TYPE_ARRAY,    // An array type implemented by PArrayType
  P_TYPE_TAG,      // A tag (referencing a declaration) type implemented by PTagType
} PTypeKind;

class PType
{
public:
  PTypeKind kind;
  PType* canonical_type;
  void* _llvm_cached_type;

  [[nodiscard]] PTypeKind get_kind() const { return kind; }
  /// Shorthand for `get_canonical_ty()->get_kind()`
  [[nodiscard]] PTypeKind get_canonical_kind() const { return get_canonical_ty()->kind; }

  /// Checks if the type it is a canonical type or not, that is if `get_canonical_ty() == this`.
  [[nodiscard]] bool is_canonical_ty() const { return canonical_type == this; }
  [[nodiscard]] PType* get_canonical_ty() const { return canonical_type; }

  /// Returns `true` if this is canonically the `void` type.
  [[nodiscard]] bool is_void_ty() const { return get_canonical_kind() == P_TYPE_VOID; }
  /// Returns `true` if this is canonically the `bool` type.
  [[nodiscard]] bool is_bool_ty() const { return get_canonical_kind() == P_TYPE_BOOL; }
  /// Returns `true` if this is canonically a pointer type.
  [[nodiscard]] bool is_pointer_ty() const { return get_canonical_kind() == P_TYPE_POINTER; }
  /// Returns `true` if this is canonically a function type.
  [[nodiscard]] bool is_function_ty() const { return get_canonical_kind() == P_TYPE_FUNCTION; }

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
    : kind(p_kind)
    , canonical_type(this)
    , _llvm_cached_type(nullptr)
  {
  }
};

/// A parenthesized type (e.g. `i32`).
/// Unlike other types, parenthesized types are unique and therefore pointer
/// comparison may not be appropriate.
class PParenType : public PType
{
public:
  PType* sub_type;

private:
  friend class PContext;
  explicit PParenType(PType* p_sub_type)
    : PType(P_TYPE_PAREN)
    , sub_type(p_sub_type)
  {
    canonical_type = p_sub_type->get_canonical_ty();
  }
};

class PFunctionType : public PType
{
public:
  [[nodiscard]] PType* get_ret_ty() const { return ret_ty; }

  PType* ret_ty;
  size_t arg_count;
  PType* args[1]; /* tail-allocated */

private:
  friend class PContext;
  explicit PFunctionType(PType* p_ret_ty, PType** p_args_ty, size_t p_arg_count)
    : PType(P_TYPE_FUNCTION)
    , arg_count(p_arg_count)
  {
    ret_ty = p_ret_ty;
    std::memcpy(args, p_args_ty, sizeof(PType*) * p_arg_count);
  }
};

class PPointerType : public PType
{
public:
  PType* element_type;

private:
  friend class PContext;
  PPointerType(PType* p_elt_ty)
    : PType(P_TYPE_POINTER)
    , element_type(p_elt_ty)
  {
  }
};

class PArrayType : public PType
{
public:
  PType* element_type;
  size_t num_elements;

private:
  friend class PContext;
  PArrayType(PType* p_elt_ty, size_t p_num_elt)
    : PType(P_TYPE_ARRAY)
    , element_type(p_elt_ty)
    , num_elements(p_num_elt)
  {
  }
};

// Type that is intimately linked to a statement. The type
// itself is defined implicitly by the attached declaration.
struct PTagType : public PType
{
  struct PDecl* decl;
};

HEDLEY_ALWAYS_INLINE static bool
p_type_is_generic_int(PType* p_type)
{
  return p_type->get_canonical_ty()->get_kind() == P_TYPE_GENERIC_INT;
}

HEDLEY_ALWAYS_INLINE static bool
p_type_is_generic_float(PType* p_type)
{
  return p_type->get_canonical_ty()->get_kind() == P_TYPE_GENERIC_FLOAT;
}

bool
p_type_is_generic(PType* p_type);
int
p_type_get_bitwidth(PType* p_type);
PType*
p_type_get_bool(void);
PType*
p_type_get_tag(struct PDecl* p_decl);
