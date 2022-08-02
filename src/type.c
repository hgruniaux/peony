#include "type.h"

#include "utils/bump_allocator.h"
#include "utils/dynamic_array.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

PType g_type_void;
PType g_type_char;
PType g_type_i8;
PType g_type_i16;
PType g_type_i32;
PType g_type_i64;
PType g_type_u8;
PType g_type_u16;
PType g_type_u32;
PType g_type_u64;
PType g_type_generic_int;
PType g_type_f32;
PType g_type_f64;
PType g_type_generic_float;
PType g_type_bool;

PDynamicArray g_func_types;
PDynamicArray g_pointer_types;
PDynamicArray g_array_types;

#define TYPE_LIST_INIT(p_array) DYN_ARRAY_INIT(PType*, p_array)
#define TYPE_LIST_DESTROY(p_array) DYN_ARRAY_DESTROY(PType*, p_array)
#define TYPE_LIST_APPEND(p_array, p_item) DYN_ARRAY_APPEND(PType*, p_array, p_item)
#define TYPE_LIST_AT(p_array, p_idx) DYN_ARRAY_AT(PType*, p_array, p_idx)

static void
init_type(PType* p_type, PTypeKind p_kind)
{
  P_TYPE_GET_KIND(p_type) = p_kind;
  p_type->common.canonical_type = p_type;
  p_type->common._llvm_cached_type = NULL;
}

void
p_init_types(void)
{
  init_type(&g_type_void, P_TYPE_VOID);
  init_type(&g_type_char, P_TYPE_CHAR);
  init_type(&g_type_i8, P_TYPE_I8);
  init_type(&g_type_i16, P_TYPE_I16);
  init_type(&g_type_i32, P_TYPE_I32);
  init_type(&g_type_i64, P_TYPE_I64);
  init_type(&g_type_u8, P_TYPE_U8);
  init_type(&g_type_u16, P_TYPE_U16);
  init_type(&g_type_u32, P_TYPE_U32);
  init_type(&g_type_u64, P_TYPE_U64);
  init_type(&g_type_generic_int, P_TYPE_GENERIC_INT);
  init_type(&g_type_f32, P_TYPE_F32);
  init_type(&g_type_f64, P_TYPE_F64);
  init_type(&g_type_generic_float, P_TYPE_GENERIC_FLOAT);
  init_type(&g_type_bool, P_TYPE_BOOL);

  TYPE_LIST_INIT(&g_func_types);
  TYPE_LIST_INIT(&g_pointer_types);
  TYPE_LIST_INIT(&g_array_types);
}

bool
p_type_is_int(PType* p_type)
{
  p_type = p_type_get_canonical(p_type);
  return p_type_is_signed(p_type) || p_type_is_unsigned(p_type);
}

bool
p_type_is_signed(PType* p_type)
{
  p_type = p_type_get_canonical(p_type);
  switch (P_TYPE_GET_KIND(p_type)) {
    case P_TYPE_I8:
    case P_TYPE_I16:
    case P_TYPE_I32:
    case P_TYPE_I64:
    case P_TYPE_GENERIC_INT: /* generic int is both signed and unsigned */
      return true;
    default:
      return false;
  }
}

bool
p_type_is_unsigned(PType* p_type)
{
  p_type = p_type_get_canonical(p_type);
  switch (P_TYPE_GET_KIND(p_type)) {
    case P_TYPE_U8:
    case P_TYPE_U16:
    case P_TYPE_U32:
    case P_TYPE_U64:
    case P_TYPE_GENERIC_INT: /* generic int is both signed and unsigned */
      return true;
    default:
      return false;
  }
}

bool
p_type_is_float(PType* p_type)
{
  p_type = p_type_get_canonical(p_type);
  switch (P_TYPE_GET_KIND(p_type)) {
    case P_TYPE_F32:
    case P_TYPE_F64:
    case P_TYPE_GENERIC_FLOAT:
      return true;
    default:
      return false;
  }
}

bool
p_type_is_arithmetic(PType* p_type)
{
  p_type = p_type_get_canonical(p_type);
  return p_type_is_int(p_type) || p_type_is_float(p_type);
}

bool
p_type_is_generic(PType* p_type)
{
  p_type = p_type_get_canonical(p_type);
  return P_TYPE_GET_KIND(p_type) == P_TYPE_GENERIC_INT || P_TYPE_GET_KIND(p_type) == P_TYPE_GENERIC_FLOAT;
}

int
p_type_get_bitwidth(PType* p_type)
{
  p_type = p_type_get_canonical(p_type);
  switch (P_TYPE_GET_KIND(p_type)) {
    case P_TYPE_BOOL:
      return 1; /* not really stored as 1-bit */
    case P_TYPE_I8:
    case P_TYPE_U8:
      return 8;
    case P_TYPE_I16:
    case P_TYPE_U16:
      return 16;
    case P_TYPE_CHAR:
    case P_TYPE_I32:
    case P_TYPE_U32:
    case P_TYPE_F32:
      return 32;
    case P_TYPE_I64:
    case P_TYPE_U64:
    case P_TYPE_F64:
      return 64;
    default:
      HEDLEY_UNREACHABLE_RETURN(-1);
  }
}

PType*
p_type_get_void()
{
  return &g_type_void;
}
PType*
p_type_get_char(void)
{
  return &g_type_char;
}
PType*
p_type_get_i8(void)
{
  return &g_type_i8;
}
PType*
p_type_get_i16(void)
{
  return &g_type_i16;
}
PType*
p_type_get_i32(void)
{
  return &g_type_i32;
}
PType*
p_type_get_i64(void)
{
  return &g_type_i64;
}
PType*
p_type_get_u8(void)
{
  return &g_type_u8;
}
PType*
p_type_get_u16(void)
{
  return &g_type_u16;
}
PType*
p_type_get_u32(void)
{
  return &g_type_u32;
}
PType*
p_type_get_u64(void)
{
  return &g_type_u64;
}
PType*
p_type_get_generic_int(void)
{
  return &g_type_generic_int;
}
PType*
p_type_get_f32(void)
{
  return &g_type_f32;
}
PType*
p_type_get_f64(void)
{
  return &g_type_f64;
}
PType*
p_type_get_generic_float(void)
{
  return &g_type_generic_float;
}
PType*
p_type_get_bool(void)
{
  return &g_type_bool;
}

PType*
p_type_get_paren(PType* p_sub_type)
{
  assert(p_sub_type != NULL);

  PParenType* type = P_BUMP_ALLOC(&p_global_bump_allocator, PParenType);
  init_type((PType*)type, P_TYPE_PAREN);
  type->sub_type = p_sub_type;
  type->common.canonical_type = p_type_get_canonical(p_sub_type);
  return (PType*)type;
}

PType*
p_type_get_function(PType* p_ret_ty, PType** p_args, int p_arg_count)
{
  assert(p_ret_ty != NULL && (p_arg_count == 0 || p_args != NULL));

  /* If the type already exists returns it (one unique instance per type). */
  for (size_t i = 0; i < g_func_types.size; ++i) {
    PFunctionType* func_type = (PFunctionType*)TYPE_LIST_AT(&g_func_types, i);
    if (func_type->ret_type == p_ret_ty && func_type->arg_count == p_arg_count &&
        memcmp(func_type->args, p_args, sizeof(PType*) * p_arg_count) == 0)
      return (PType*)func_type;
  }

  PFunctionType* func_type =
    p_bump_alloc(&p_global_bump_allocator, sizeof(PFunctionType) + sizeof(PType*) * p_arg_count, P_ALIGNOF(PType));
  init_type((PType*)func_type, P_TYPE_FUNCTION);
  func_type->ret_type = p_ret_ty;
  func_type->arg_count = p_arg_count;
  memcpy(func_type->args, p_args, sizeof(PType*) * p_arg_count);

  // Verify if this function type is canonical (that is if its return type and
  // its arguments are all canonicals).
  bool is_canonical = p_type_is_canonical(p_ret_ty);
  if (is_canonical) {
    for (size_t i = 0; i < p_arg_count; ++i) {
      if (!p_type_is_canonical(p_args[i])) {
        is_canonical = false;
        break;
      }
    }
  }

  // If not canonical then compute its canonical type.
  if (!is_canonical) {
    PType** canonical_args = malloc(sizeof(PType*) * p_arg_count);
    assert(canonical_args != NULL);

    for (size_t i = 0; i < p_arg_count; ++i) {
      canonical_args[i] = p_type_get_canonical(p_args[i]);
    }

    func_type->common.canonical_type = p_type_get_function(p_type_get_canonical(p_ret_ty), canonical_args, p_arg_count);

    free(canonical_args);
  }

  TYPE_LIST_APPEND(&g_func_types, func_type);
  return (PType*)func_type;
}

PType*
p_type_get_pointer(PType* p_element_ty)
{
  assert(p_element_ty != NULL);

  /* If the type already exists returns it (one unique instance per type). */
  for (size_t i = 0; i < g_pointer_types.size; ++i) {
    PPointerType* ptr_type = (PPointerType*)TYPE_LIST_AT(&g_pointer_types, i);
    if (ptr_type->element_type == p_element_ty)
      return (PType*)ptr_type;
  }

  PPointerType* ptr_type = P_BUMP_ALLOC(&p_global_bump_allocator, PPointerType);
  init_type((PType*)ptr_type, P_TYPE_POINTER);
  ptr_type->element_type = p_element_ty;

  if (!p_type_is_canonical(p_element_ty))
    ptr_type->common.canonical_type = p_type_get_pointer(p_type_get_canonical(p_element_ty));

  TYPE_LIST_APPEND(&g_pointer_types, ptr_type);
  return (PType*)ptr_type;
}

PType*
p_type_get_array(PType* p_element_ty, int p_num_elements)
{
  assert(p_element_ty != NULL && p_num_elements > 0);

  /* If the type already exists returns it (one unique instance per type). */
  for (size_t i = 0; i < g_array_types.size; ++i) {
    PArrayType* array_type = (PArrayType*)TYPE_LIST_AT(&g_array_types, i);
    if (array_type->element_type == p_element_ty && array_type->num_elements == p_num_elements)
      return (PType*)array_type;
  }

  PArrayType* array_type = P_BUMP_ALLOC(&p_global_bump_allocator, PArrayType);
  init_type((PType*)array_type, P_TYPE_ARRAY);
  array_type->element_type = p_element_ty;
  array_type->num_elements = p_num_elements;

   if (!p_type_is_canonical(p_element_ty))
    array_type->common.canonical_type = p_type_get_array(p_type_get_canonical(p_element_ty), p_num_elements);

  TYPE_LIST_APPEND(&g_array_types, array_type);
  return (PType*)array_type;
}
