#include "context.hxx"

#include <cassert>

PContext&
PContext::get_global()
{
  static PContext g_instance;
  return g_instance;
}

PParenType*
PContext::get_paren_ty(PType* p_sub_type)
{
  assert(p_sub_type != nullptr);

  // We don't bother to unique parenthesized types.
  auto* type = alloc_object<PParenType>();
  new (type) PParenType(p_sub_type);
  return type;
}

static bool
is_func_ty_canonical(PType* p_ret_ty, PType** p_args, size_t p_arg_count)
{
  bool is_canonical = p_ret_ty->is_canonical_ty();
  for (size_t i = 0; i < p_arg_count; ++i) {
    if (p_args[i] != nullptr)
      is_canonical &= p_args[i]->is_canonical_ty();
  }

  return is_canonical;
}

PFunctionType*
PContext::get_function_ty(PType* p_ret_ty, PType** p_params, size_t p_param_count)
{
  assert(p_ret_ty != nullptr && (p_params != nullptr || p_param_count == 0));

  // If the type already exists return it.
  const auto key = FuncTyKey{ p_ret_ty, p_params, p_param_count };
  const auto it = m_func_tys.find(key);
  if (it != m_func_tys.end())
    return it->second;

  auto** raw_params = m_allocator.alloc_object<PType*>(p_param_count);
  memcpy(raw_params, p_params, sizeof(PType*) * p_param_count);

  auto* type = m_allocator.alloc_object<PFunctionType>();
  new (type) PFunctionType(p_ret_ty, raw_params, p_param_count);

  if (!is_func_ty_canonical(p_ret_ty, p_params, p_param_count)) {
    PType* can_ret_ty = p_ret_ty->get_canonical_ty();
    std::vector<PType*> can_args(p_param_count);
    for (size_t i = 0; i < p_param_count; ++i) {
      can_args[i] = p_params[i]->get_canonical_ty();
    }

    type->m_canonical_type = get_function_ty(can_ret_ty, can_args);
  }

  m_func_tys.insert({ FuncTyKey{ type->get_ret_ty(), type->get_params(), type->get_param_count() }, type });
  return type;
}

PFunctionType*
PContext::get_function_ty(PType* p_ret_ty, const std::vector<PType*>& p_args)
{
  // Because PType are immutable `const_cast`ing them does not imply anything.
  return get_function_ty(p_ret_ty, const_cast<PType**>(p_args.data()), p_args.size());
}

PPointerType*
PContext::get_pointer_ty(PType* p_elt_ty)
{
  assert(p_elt_ty != nullptr);

  // If the type already exists return it.
  const auto it = m_pointer_tys.find(p_elt_ty);
  if (it != m_pointer_tys.end())
    return it->second;

  auto* type = alloc_object<PPointerType>();
  new (type) PPointerType(p_elt_ty);
  if (!p_elt_ty->is_canonical_ty())
    type->m_canonical_type = get_pointer_ty(p_elt_ty->get_canonical_ty());

  m_pointer_tys.insert({ p_elt_ty, type });
  return type;
}

PArrayType*
PContext::get_array_ty(PType* p_elt_ty, size_t p_num_elements)
{
  assert(p_elt_ty != nullptr);

  // If the type already exists return it.
  const auto it = m_array_tys.find(std::make_pair(p_elt_ty, p_num_elements));
  if (it != m_array_tys.end())
    return it->second;

  auto* type = alloc_object<PArrayType>();
  new (type) PArrayType(p_elt_ty, p_num_elements);
  if (!p_elt_ty->is_canonical_ty())
    type->m_canonical_type = get_array_ty(p_elt_ty->get_canonical_ty(), p_num_elements);

  m_array_tys.insert({ { p_elt_ty, p_num_elements }, type });
  return type;
}