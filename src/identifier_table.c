#include "identifier_table.h"

#include "utils/bump_allocator.h"
#include "utils/hash_table_common.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Computes a hash value from the given string. */
static size_t
compute_hash(const char* p_begin, const char* p_end)
{
  if (sizeof(size_t) == 8) {
    /*
     * 64-bit FNV-1a
     * Implementation from: http://www.isthe.com/chongo/tech/comp/fnv/index.html
     * Under public domain
     */

    size_t hval = 0xcbf29ce484222325ULL;

    while (p_begin < p_end) {
      hval ^= (size_t)*p_begin++;
      hval += (hval << 1) + (hval << 4) + (hval << 5) + (hval << 7) +
              (hval << 8) + (hval << 40);
    }

    return hval;
  } else if (sizeof(size_t) == 4) {
    /*
     * 32-bit FNV-1a
     * Implementation from: http://www.isthe.com/chongo/tech/comp/fnv/index.html
     * Under public domain
     */

    size_t hval = 0x811c9dc5;

    while (p_begin < p_end) {
      hval ^= (size_t)*p_begin++;
      hval +=
        (hval << 1) + (hval << 4) + (hval << 7) + (hval << 8) + (hval << 24);
    }

    return hval;
  } else {
    /*
     * Dummy string hash (djb2).
     * Implementation from: http://www.cse.yorku.ca/~oz/hash.html
     */

    size_t hval = 5381;

    while (p_begin < p_end) {
      hval = ((hval << 5) + hval) ^ (size_t)*p_begin++;
    }

    return hval;
  }
}

static bool
is_same_identifier(struct PIdentifierInfo* p_identifier_info,
                   const char* p_spelling_begin,
                   const char* p_spelling_end)
{
  size_t spelling_len = p_spelling_end - p_spelling_begin;
  if (spelling_len != p_identifier_info->spelling_len)
    return false;

  return memcmp(p_identifier_info->spelling,
                p_spelling_begin,
                sizeof(char) * p_identifier_info->spelling_len) == 0;
}

void
p_identifier_table_init(struct PIdentifierTable* p_table)
{
  assert(p_table != NULL);

  p_table->bucket_count = p_get_new_hash_table_size(0);
  p_table->identifiers =
    P_ALLOC_BUCKETS(p_table->bucket_count, struct PIdentifierInfo*);

  p_table->item_count = 0;
  p_table->rehashing_count = 0;
}

void
p_identifier_table_destroy(struct PIdentifierTable* p_table)
{
  assert(p_table != NULL);

  free(p_table->identifiers);
}

void
p_identifier_table_dump_stats(struct PIdentifierTable* p_table)
{
  assert(p_table != NULL);

  printf("Identifier table stats:\n");
  printf("  bucket_count: %zu\n", p_table->bucket_count);
  printf("  item_count: %zu\n", p_table->item_count);
  printf("  load_factor: %f (max: %f)\n",
         P_GET_LOAD_FACTOR(p_table),
         P_MAX_LOAD_FACTOR);
  printf("  rehashing_count: %zu\n", p_table->rehashing_count);
}

/* Inserts a given identifier info into the table. This function assumes
 * that the given identifier info DOES NOT exist already in the table and
 * that the table has enough space (no rehashing is done). */
static void
insert_item(struct PIdentifierTable* p_table,
            struct PIdentifierInfo* p_identifier_info)
{
  size_t bucket_idx = p_identifier_info->hash % p_table->bucket_count;
  /* This loop is guaranteed to terminate because the load factor < 1. */
  while (p_table->identifiers[bucket_idx] != NULL)
    bucket_idx = (bucket_idx + 1) % p_table->bucket_count;

  p_table->identifiers[bucket_idx] = p_identifier_info;
}

static void
rehash_table(struct PIdentifierTable* p_table)
{
  const size_t old_bucket_count = p_table->bucket_count;
  struct PIdentifierInfo** old_hash_table = p_table->identifiers;

  p_table->bucket_count = p_get_new_hash_table_size(p_table->bucket_count);
  p_table->identifiers =
    P_ALLOC_BUCKETS(p_table->bucket_count, struct PIdentifierInfo*);
  for (size_t i = 0; i < old_bucket_count; ++i) {
    if (old_hash_table[i] != NULL)
      insert_item(p_table, old_hash_table[i]);
  }

  free(old_hash_table);
  p_table->rehashing_count++;
}

static struct PIdentifierInfo*
ident_info_new(const char* p_spelling_begin,
               const char* p_spelling_end,
               size_t p_hash)
{
  const size_t spelling_len = p_spelling_end - p_spelling_begin;
  struct PIdentifierInfo* identifier_info =
    p_bump_alloc(&p_global_bump_allocator,
                 sizeof(struct PIdentifierInfo) + sizeof(char) * spelling_len,
                 P_ALIGNOF(struct PIdentifierInfo));
  identifier_info->hash = p_hash;
  identifier_info->token_kind = P_TOK_IDENTIFIER;
  identifier_info->spelling_len = spelling_len;
  memcpy(
    identifier_info->spelling, p_spelling_begin, sizeof(char) * spelling_len);
  identifier_info->spelling[spelling_len] = '\0';

  return identifier_info;
}

struct PIdentifierInfo*
p_identifier_table_get(struct PIdentifierTable* p_table,
                       const char* p_spelling_begin,
                       const char* p_spelling_end)
{
  assert(p_table != NULL && p_spelling_begin != NULL);

  if (p_spelling_end == NULL) {
    const size_t spelling_len = strlen(p_spelling_begin);
    p_spelling_end = p_spelling_begin + spelling_len;
  }

  const size_t hash = compute_hash(p_spelling_begin, p_spelling_end);
  size_t bucket_idx = hash % p_table->bucket_count;

  /* Try to find the identifier if it was already registered: */
  while (p_table->identifiers[bucket_idx] != NULL) {
    if (is_same_identifier(
          p_table->identifiers[bucket_idx], p_spelling_begin, p_spelling_end)) {
      return p_table->identifiers[bucket_idx];
    }

    bucket_idx++;
    if (bucket_idx >= p_table->bucket_count)
      bucket_idx = 0;
  }

  /* Otherwise, insert it: */
  struct PIdentifierInfo* identifier_info =
    ident_info_new(p_spelling_begin, p_spelling_end, hash);

  if (!P_NEEDS_REHASHING(p_table)) {
    p_table->identifiers[bucket_idx] = identifier_info;
  } else {
    rehash_table(p_table);
    insert_item(p_table, identifier_info);
  }

  return identifier_info;
}

void
p_identifier_table_register_keywords(struct PIdentifierTable* p_table)
{
  assert(p_table != NULL);

#define TOKEN(p_kind)
#define KEYWORD(p_spelling)                                                    \
  p_identifier_table_get(p_table, #p_spelling, NULL)->token_kind =             \
    P_TOK_KEY_##p_spelling;
#include "token_kinds.def"
}
