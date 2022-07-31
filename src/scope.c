#include "scope.h"

#include "utils/hash_table_common.h"
#include "utils/bump_allocator.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

void
p_scope_init(PScope* p_scope, PScope* p_parent_scope)
{
  assert(p_scope != NULL);

  p_scope->parent_scope = p_parent_scope;
  p_scope->bucket_count = p_get_new_hash_table_size(0);
  p_scope->symbols = P_ALLOC_BUCKETS(p_scope->bucket_count, PSymbol*);
  p_scope->item_count = 0;
}

void
p_scope_destroy(PScope* p_scope)
{
  assert(p_scope != NULL);

  free(p_scope->symbols);
  p_scope->symbols = NULL;
  p_scope->bucket_count = 0;
  p_scope->item_count = 0;
}

void
p_scope_clear(PScope* p_scope)
{
  assert(p_scope != NULL);

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
  while (p_table->symbols[bucket_idx] != NULL)
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
    if (old_hash_table[i] != NULL)
      insert_item(p_table, old_hash_table[i]);
  }

  free(old_hash_table);
}

PSymbol*
p_scope_local_lookup(PScope* p_scope, PIdentifierInfo* p_name)
{
  assert(p_scope != NULL && p_name != NULL);

  size_t bucket_idx = p_name->hash % p_scope->bucket_count;

  while (p_scope->symbols[bucket_idx] != NULL) {
    if (p_scope->symbols[bucket_idx]->name == p_name)
      return p_scope->symbols[bucket_idx];

    bucket_idx++;
    if (bucket_idx >= p_scope->bucket_count)
      bucket_idx = 0;
  }

  return NULL;
}

PSymbol*
p_scope_add_symbol(PScope* p_scope, PIdentifierInfo* p_name)
{
  assert(p_scope != NULL && p_name != NULL);

  size_t bucket_idx = p_name->hash % p_scope->bucket_count;

  /* Try to find a free slot: */
  while (p_scope->symbols[bucket_idx] != NULL) {
    /* The symbol must not be already registered. */
    assert(p_scope->symbols[bucket_idx]->name != p_name);

    bucket_idx++;
    if (bucket_idx >= p_scope->bucket_count)
      bucket_idx = 0;
  }

  PSymbol* symbol =
    P_BUMP_ALLOC(&p_global_bump_allocator, PSymbol);
  symbol->scope = p_scope;
  symbol->name = p_name;

  /* Insert the symbol into the hash table: */
  if (!P_NEEDS_REHASHING(p_scope)) {
    p_scope->symbols[bucket_idx] = symbol;
  } else {
    rehash_table(p_scope);
    insert_item(p_scope, symbol);
  }

  return symbol;
}
