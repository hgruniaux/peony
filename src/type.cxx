#include "type.hxx"

#include "context.hxx"
#include "utils/bump_allocator.hxx"

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <vector>

std::vector<PFunctionType*> g_func_types;
std::vector<PPointerType*> g_pointer_types;
std::vector<PArrayType*> g_array_types;

static void
init_type(PType* p_type, PTypeKind p_kind)
{
  p_type->kind = p_kind;
  p_type->canonical_type = p_type;
  p_type->_llvm_cached_type = nullptr;
}

bool
p_type_is_generic(PType* p_type)
{
  p_type = p_type->get_canonical_ty();
  return p_type->get_kind() == P_TYPE_GENERIC_INT || p_type->get_kind() == P_TYPE_GENERIC_FLOAT;
}

int
p_type_get_bitwidth(PType* p_type)
{
  p_type = p_type->get_canonical_ty();
  switch (p_type->get_kind()) {
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
p_type_get_bool(void)
{
  return PContext::get_global().get_bool_ty();
}

PType*
p_type_get_tag(struct PDecl* p_decl)
{
  auto* type = p_global_bump_allocator.alloc<PTagType>();
  init_type((PType*)type, P_TYPE_TAG);
  type->decl = p_decl;
  return (PType*)type;
}

bool
PType::is_signed_int_ty() const
{
  switch (get_canonical_kind()) {
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
PType::is_unsigned_int_ty() const
{
  switch (get_canonical_kind()) {
    case P_TYPE_U8:
    case P_TYPE_U16:
    case P_TYPE_U32:
    case P_TYPE_U64:
      return true;
    default:
      return false;
  }
}
