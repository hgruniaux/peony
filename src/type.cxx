#include "type.hxx"
#include "context.hxx"

#include <cassert>

bool
PType::is_signed_int_ty() const
{
  switch (get_canonical_kind()) {
    case P_TK_I8:
    case P_TK_I16:
    case P_TK_I32:
    case P_TK_I64:
      return true;
    default:
      return false;
  }
}

bool
PType::is_unsigned_int_ty() const
{
  switch (get_canonical_kind()) {
    case P_TK_U8:
    case P_TK_U16:
    case P_TK_U32:
    case P_TK_U64:
      return true;
    default:
      return false;
  }
}

bool
PType::is_float_ty() const
{
  switch (get_canonical_kind()) {
    case P_TK_F32:
    case P_TK_F64:
      return true;
    default:
      return false;
  }
}

PType*
PType::to_signed_int_ty(PContext& p_ctx) const
{
  switch (get_canonical_kind()) {
    case P_TK_U8:
      return p_ctx.get_i8_ty();
    case P_TK_U16:
      return p_ctx.get_i16_ty();
    case P_TK_U32:
      return p_ctx.get_i32_ty();
    case P_TK_U64:
      return p_ctx.get_i64_ty();
    case P_TK_I8:
    case P_TK_I16:
    case P_TK_I32:
    case P_TK_I64:
      return const_cast<PType*>(this); // Already signed
    default:
      assert(false && "not an integer type");
      return nullptr;
  }
}

PType*
PType::to_unsigned_int_ty(PContext& p_ctx) const
{
  switch (get_canonical_kind()) {
    case P_TK_I8:
      return p_ctx.get_u8_ty();
    case P_TK_I16:
      return p_ctx.get_u16_ty();
    case P_TK_I32:
      return p_ctx.get_u32_ty();
    case P_TK_I64:
      return p_ctx.get_u64_ty();
    case P_TK_U8:
    case P_TK_U16:
    case P_TK_U32:
    case P_TK_U64:
      return const_cast<PType*>(this); // Already unsigned
    default:
      assert(false && "not an integer type");
      return nullptr;
  }
}
