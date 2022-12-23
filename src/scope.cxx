#include "scope.hxx"

#include "utils/hash_table_common.hxx"
#include "utils/bump_allocator.hxx"

#include <cassert>
#include <cstdlib>
#include <cstring>

void
p_scope_init(PScope* p_scope, PScope* p_parent_scope, PScopeFlags p_flags)
{
  assert(p_scope != nullptr);

  p_scope->parent_scope = p_parent_scope;
  p_scope->statement = nullptr;
  p_scope->bucket_count = p_get_new_hash_table_size(0);
  p_scope->symbols = P_ALLOC_BUCKETS(p_scope->bucket_count, PSymbol*);
  p_scope->item_count = 0;
  p_scope->flags = p_flags;
}

void
p_scope_destroy(PScope* p_scope)
{
  assert(p_scope != nullptr);

  free(p_scope->symbols);
  p_scope->symbols = nullptr;
  p_scope->bucket_count = 0;
  p_scope->item_count = 0;
}

void
p_scope_clear(PScope* p_scope)
{
  assert(p_scope != nullptr);

  p_scope->item_count = 0;
  memset(p_scope->symbols, 0, sizeof(PSymbol*) * p_scope->bucket_count);
}

/* Inserts a given identifier info into the table. This function assumes
 * that the given identifier info DOES NOT exist already in the table and
 * that the table has enough space (no rehashing is done). */
static void
insert_item(PScope* p_table, PSymbol* p_symbol)
{
  size_t bucket_idx = p_symbol->name->hash % p_table->bucket_count;
  /* This loop is guaranteed to terminate because the load factor < 1. */
  while (p_table->symbols[bucket_idx] != nullptr)
    bucket_idx = (bucket_idx + 1) % p_table->bucket_count;

  p_table->symbols[bucket_idx] = p_symbol;
}

static void
rehash_table(PScope* p_table)
{
  const size_t old_bucket_count = p_table->bucket_count;
  PSymbol** old_hash_table = p_table->symbols;

  p_table->bucket_count = p_get_new_hash_table_size(p_table->bucket_count);
  p_table->symbols = P_ALLOC_BUCKETS(p_table->bucket_count, PSymbol*);
  for (size_t i = 0; i < old_bucket_count; ++i) {
    if (old_hash_table[i] != nullptr)
      insert_item(p_table, old_hash_table[i]);
  }

  free(old_hash_table);
}

PSymbol*
p_scope_local_lookup(PScope* p_scope, PIdentifierInfo* p_name)
{
  assert(p_scope != nullptr && p_name != nullptr);

  size_t bucket_idx = p_name->hash % p_scope->bucket_count;

  while (p_scope->symbols[bucket_idx] != nullptr) {
    if (p_scope->symbols[bucket_idx]->name == p_name)
      return p_scope->symbols[bucket_idx];

    bucket_idx++;
    if (bucket_idx >= p_scope->bucket_count)
      bucket_idx = 0;
  }

  return nullptr;
}

PSymbol*
p_scope_add_symbol(PScope* p_scope, PIdentifierInfo* p_name)
{
  assert(p_scope != nullptr && p_name != nullptr);

  size_t bucket_idx = p_name->hash % p_scope->bucket_count;

  /* Try to find a free slot: */
  while (p_scope->symbols[bucket_idx] != nullptr) {
    /* The symbol must not be already registered. */
    assert(p_scope->symbols[bucket_idx]->name != p_name);

    bucket_idx++;
    if (bucket_idx >= p_scope->bucket_count)
      bucket_idx = 0;
  }

  auto* symbol = p_global_bump_allocator.alloc<PSymbol>();
  symbol->scope = p_scope;
  symbol->name = p_name;

  ++p_scope->item_count;

  /* Insert the symbol into the hash table: */
  if (!P_NEEDS_REHASHING(p_scope)) {
    p_scope->symbols[bucket_idx] = symbol;
  } else {
    rehash_table(p_scope);
    insert_item(p_scope, symbol);
  }

  return symbol;
}
