#include "type.h"

#include "bump_allocator.h"
#include "hedley.h"
#include "utils/dynamic_array.h"

#include <assert.h>
#include <string.h>

PType g_type_undef;
PType g_type_void;
PType g_type_i8;
PType g_type_i16;
PType g_type_i32;
PType g_type_i64;
PType g_type_u8;
PType g_type_u16;
PType g_type_u32;
PType g_type_u64;
PType g_type_f32;
PType g_type_f64;
PType g_type_bool;

PDynamicArray g_func_types;

static void
init_type(PType* p_type, PTypeKind p_kind)
{
  P_TYPE_GET_KIND(p_type) = p_kind;
  p_type->common._llvm_cached_type = NULL;
}

void
p_init_types(void)
{
  init_type(&g_type_undef, P_TYPE_UNDEF);
  init_type(&g_type_void, P_TYPE_VOID);
  init_type(&g_type_i8, P_TYPE_I8);
  init_type(&g_type_i16, P_TYPE_I16);
  init_type(&g_type_i32, P_TYPE_I32);
  init_type(&g_type_i64, P_TYPE_I64);
  init_type(&g_type_u8, P_TYPE_U8);
  init_type(&g_type_u16, P_TYPE_U16);
  init_type(&g_type_u32, P_TYPE_U32);
  init_type(&g_type_u64, P_TYPE_U64);
  init_type(&g_type_f32, P_TYPE_F32);
  init_type(&g_type_f64, P_TYPE_F64);
  init_type(&g_type_bool, P_TYPE_BOOL);

  p_dynamic_array_init(&g_func_types);
}

bool
p_type_is_int(PType* p_type)
{
  return p_type_is_signed(p_type) || p_type_is_unsigned(p_type);
}

bool
p_type_is_signed(PType* p_type)
{
  assert(p_type != NULL);
  switch (P_TYPE_GET_KIND(p_type)) {
    case P_TYPE_I8:
    case P_TYPE_I16:
    case P_TYPE_I32:
    case P_TYPE_I64:
      return true;
    default:
      return false;
  }
}

bool
p_type_is_unsigned(PType* p_type)
{
  assert(p_type != NULL);
  switch (P_TYPE_GET_KIND(p_type)) {
    case P_TYPE_U8:
    case P_TYPE_U16:
    case P_TYPE_U32:
    case P_TYPE_U64:
      return true;
    default:
      return false;
  }
}

bool
p_type_is_float(PType* p_type)
{
  assert(p_type != NULL);
  switch (P_TYPE_GET_KIND(p_type)) {
    case P_TYPE_F32:
    case P_TYPE_F64:
      return true;
    default:
      return false;
  }
}

int
p_type_get_bitwidth(PType* p_type)
{
  switch (P_TYPE_GET_KIND(p_type)) {
    case P_TYPE_BOOL:
      return 1; /* not really stored as 1-bit */
    case P_TYPE_I8:
    case P_TYPE_U8:
      return 8;
    case P_TYPE_I16:
    case P_TYPE_U16:
      return 16;
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
p_type_get_undef(void)
{
  return &g_type_undef;
}
PType*
p_type_get_void()
{
  return &g_type_void;
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
p_type_get_bool(void)
{
  return &g_type_bool;
}

PType*
p_type_get_function(PType* p_ret_ty, PType** p_args, int p_arg_count)
{
  assert(p_ret_ty != NULL && (p_arg_count == 0 || p_args != NULL));

  /* If the type already exists returns it (each type is unique). */
  for (int i = 0; i < g_func_types.size; ++i) {
    PFunctionType* func_type = g_func_types.buffer[i];
    if (func_type->ret == p_ret_ty && func_type->arg_count == p_arg_count &&
        memcmp(func_type->args, p_args, sizeof(PType*) * p_arg_count) == 0)
      return (PType*)func_type;
  }

  PFunctionType* func_type =
    p_bump_alloc(&p_global_bump_allocator, sizeof(PFunctionType) + sizeof(PType*) * p_arg_count, P_ALIGNOF(PType));
  init_type((PType*)func_type, P_TYPE_FUNC);
  func_type->ret = p_ret_ty;
  func_type->arg_count = p_arg_count;
  memcpy(func_type->args, p_args, sizeof(PType*) * p_arg_count);

  p_dynamic_array_append(&g_func_types, func_type);
  return (PType*)func_type;
}
