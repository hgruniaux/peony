#include "scope.hxx"

#include "context.hxx"
#include "utils/bump_allocator.hxx"
#include "utils/hash_table_common.hxx"

#include <cassert>
#include <cstdlib>
#include <cstring>

PScope::PScope(PScope* p_parent_scope, PScopeFlags p_flags)
  : parent_scope(p_parent_scope)
  , statement(nullptr)
  , flags(p_flags)
{
}

PSymbol*
p_scope_local_lookup(PScope* p_scope, PIdentifierInfo* p_name)
{
  if (p_name == nullptr)
    return nullptr;

  const auto it = p_scope->symbols.find(p_name);
  if (it != p_scope->symbols.end())
    return it->second;

  return nullptr;
}

PSymbol*
p_scope_add_symbol(PScope* p_scope, PIdentifierInfo* p_name)
{
  assert(p_name != nullptr);

  auto* symbol = PContext::get_global().new_object<PSymbol>(p_scope, p_name);
  p_scope->symbols.insert({ p_name, symbol });
  return symbol;
}
