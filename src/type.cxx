#include "type.hxx"

#include "context.hxx"
#include "utils/bump_allocator.hxx"

#include <cassert>

int
p_type_get_bitwidth(PType* p_type)
{
  p_type = p_type->get_canonical_ty();
  switch (p_type->get_kind()) {
    case P_TK_BOOL:
      return 1; /* not really stored as 1-bit */
    case P_TK_I8:
    case P_TK_U8:
      return 8;
    case P_TK_I16:
    case P_TK_U16:
      return 16;
    case P_TK_CHAR:
    case P_TK_I32:
    case P_TK_U32:
    case P_TK_F32:
      return 32;
    case P_TK_I64:
    case P_TK_U64:
    case P_TK_F64:
      return 64;
    default:
      assert(false && "unimplemented for this type");
  }
}

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
