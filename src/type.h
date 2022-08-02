#pragma once

#include <hedley.h>

#include <stdbool.h>

HEDLEY_BEGIN_C_DECLS

typedef enum PTypeKind
{
  P_TYPE_VOID,
  P_TYPE_CHAR, /** @brief Unicode scalar value. */
  P_TYPE_I8,
  P_TYPE_I16,
  P_TYPE_I32,
  P_TYPE_I64,
  P_TYPE_U8,
  P_TYPE_U16,
  P_TYPE_U32,
  P_TYPE_U64,
  P_TYPE_GENERIC_INT,
  P_TYPE_F32,
  P_TYPE_F64,
  P_TYPE_GENERIC_FLOAT,
  P_TYPE_BOOL,
  P_TYPE_PAREN, /** @brief Parenthesized type (e.g. `(i32)`), unlike other types they are not unique. */
  P_TYPE_FUNCTION,
  P_TYPE_POINTER,
  P_TYPE_ARRAY,
  P_TYPE_TAG,
} PTypeKind;

typedef struct PTypeCommon
{
  PTypeKind kind;
  struct PType* canonical_type;
  void* _llvm_cached_type;
} PTypeCommon;

typedef struct PType
{
  PTypeCommon common;
} PType;

typedef struct PParenType
{
  PTypeCommon common;
  PType* sub_type;
} PParenType;

typedef struct PFunctionType
{
  PTypeCommon common;
  PType* ret_type;
  int arg_count;
  PType* args[1]; /* tail-allocated */
} PFunctionType;

typedef struct PPointerType
{
  PTypeCommon common;
  PType* element_type;
} PPointerType;

typedef struct PArrayType
{
  PTypeCommon common;
  PType* element_type;
  int num_elements;
} PArrayType;

// Type that is intimately linked to a statement. The type
// itself is defined implicitly by the attached declaration.
typedef struct PTagType
{
  PTypeCommon common;
  struct PDecl* decl;
} PTagType;

#define P_TYPE_GET_KIND(p_type) (((PType*)(p_type))->common.kind)

void
p_init_types(void);

/** @brief Checks if the type is already a canonical type. */
HEDLEY_ALWAYS_INLINE static bool
p_type_is_canonical(PType* p_type)
{
  return p_type == p_type->common.canonical_type;
}
/** @brief Gets the canonical type of the given type.
 *
 * Canonical types are cached so this function is just a pointer access. */
HEDLEY_ALWAYS_INLINE static PType*
p_type_get_canonical(PType* p_type)
{
  return p_type->common.canonical_type;
}

HEDLEY_ALWAYS_INLINE static bool
p_type_is_bool(PType* p_type)
{
  return P_TYPE_GET_KIND(p_type_get_canonical(p_type)) == P_TYPE_BOOL;
}

HEDLEY_ALWAYS_INLINE static bool
p_type_is_void(PType* p_type)
{
  return P_TYPE_GET_KIND(p_type_get_canonical(p_type)) == P_TYPE_VOID;
}

HEDLEY_ALWAYS_INLINE static bool
p_type_is_char(PType* p_type)
{
  return P_TYPE_GET_KIND(p_type_get_canonical(p_type)) == P_TYPE_CHAR;
}

HEDLEY_ALWAYS_INLINE static bool
p_type_is_pointer(PType* p_type)
{
  return P_TYPE_GET_KIND(p_type_get_canonical(p_type)) == P_TYPE_POINTER;
}

HEDLEY_ALWAYS_INLINE static bool
p_type_is_array(PType* p_type)
{
  return P_TYPE_GET_KIND(p_type_get_canonical(p_type)) == P_TYPE_ARRAY;
}

HEDLEY_ALWAYS_INLINE static bool
p_type_is_function(PType* p_type)
{
  return P_TYPE_GET_KIND(p_type_get_canonical(p_type)) == P_TYPE_FUNCTION;
}

HEDLEY_ALWAYS_INLINE static bool
p_type_is_generic_int(PType* p_type)
{
  return P_TYPE_GET_KIND(p_type_get_canonical(p_type)) == P_TYPE_GENERIC_INT;
}

HEDLEY_ALWAYS_INLINE static bool
p_type_is_generic_float(PType* p_type)
{
  return P_TYPE_GET_KIND(p_type_get_canonical(p_type)) == P_TYPE_GENERIC_FLOAT;
}

bool
p_type_is_int(PType* p_type);
bool
p_type_is_signed(PType* p_type);
bool
p_type_is_unsigned(PType* p_type);
bool
p_type_is_float(PType* p_type);
bool
p_type_is_arithmetic(PType* p_type);
bool
p_type_is_generic(PType* p_type);

int
p_type_get_bitwidth(PType* p_type);

PType*
p_type_get_void(void);
PType*
p_type_get_char(void);
PType*
p_type_get_i8(void);
PType*
p_type_get_i16(void);
PType*
p_type_get_i32(void);
PType*
p_type_get_i64(void);
PType*
p_type_get_u8(void);
PType*
p_type_get_u16(void);
PType*
p_type_get_u32(void);
PType*
p_type_get_u64(void);
PType*
p_type_get_generic_int(void);
PType*
p_type_get_f32(void);
PType*
p_type_get_f64(void);
PType*
p_type_get_generic_float(void);
PType*
p_type_get_bool(void);

PType*
p_type_get_paren(PType* p_sub_type);

PType*
p_type_get_function(PType* p_ret_ty, PType** p_args, int p_arg_count);

PType*
p_type_get_pointer(PType* p_element_ty);

PType*
p_type_get_array(PType* p_element_ty, int p_num_elements);

PType*
p_type_get_tag(struct PDecl* p_decl);

HEDLEY_END_C_DECLS
