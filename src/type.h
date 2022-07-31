#pragma once

#include <stdbool.h>

typedef enum PTypeKind
{
  P_TYPE_VOID,
  P_TYPE_UNDEF,
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
  P_TYPE_BOOL,
  P_TYPE_FUNC
} PTypeKind;

typedef struct PTypeCommon
{
  PTypeKind kind;
  void* _llvm_cached_type;
} PTypeCommon;

typedef struct PType
{
  PTypeCommon common;
} PType;

typedef struct PFunctionType
{
  PTypeCommon common;
  PType* ret;
  int arg_count;
  PType* args[1]; /* tail-allocated */
} PFunctionType;

#define P_TYPE_GET_KIND(p_type) (((PType*)(p_type))->common.kind)

void
p_init_types(void);

#define p_type_is_bool(p_type) (P_TYPE_GET_KIND(p_type) == P_TYPE_BOOL)
#define p_type_is_void(p_type) (P_TYPE_GET_KIND(p_type) == P_TYPE_VOID)

bool
p_type_is_int(PType* p_type);
bool
p_type_is_signed(PType* p_type);
bool
p_type_is_unsigned(PType* p_type);
bool
p_type_is_float(PType* p_type);

int
p_type_get_bitwidth(PType* p_type);

PType*
p_type_get_undef(void);
PType*
p_type_get_void(void);
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
p_type_get_f32(void);
PType*
p_type_get_f64(void);
PType*
p_type_get_bool(void);

PType*
p_type_get_function(PType* p_ret_ty, PType** p_args, int p_arg_count);
