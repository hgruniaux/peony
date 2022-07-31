#include "type.h"

#include "bump_allocator.h"

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

PType* g_func_types;

static void
init_type(PType* p_type, PTypeKind p_kind)
{
  p_type->kind = p_kind;
  p_type->_next = NULL;
  p_type->_llvm_cached_type = NULL;
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

  g_func_types = NULL;
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
  switch (p_type->kind) {
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
  switch (p_type->kind) {
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
  switch (p_type->kind) {
    case P_TYPE_F32:
    case P_TYPE_F64:
      return true;
    default:
      return false;
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
  PType* func_ty = g_func_types;
  while (func_ty != NULL) {
    if (func_ty->func_ty.ret == p_ret_ty && func_ty->func_ty.arg_count == p_arg_count &&
        memcmp(func_ty->func_ty.args, p_args, sizeof(PType*) * p_arg_count) == 0)
      return func_ty;

    func_ty = func_ty->_next;
  }

  func_ty = p_bump_alloc(&p_global_bump_allocator, sizeof(PType) + sizeof(PType*) * p_arg_count, P_ALIGNOF(PType));
  init_type(func_ty, P_TYPE_FUNC);
  func_ty->func_ty.ret = p_ret_ty;
  func_ty->func_ty.arg_count = p_arg_count;
  memcpy(func_ty->func_ty.args, p_args, sizeof(PType*) * p_arg_count);

  if (g_func_types != NULL) {
    g_func_types->_next = func_ty;
  } else {
    g_func_types = func_ty;
  }

  return func_ty;
}
