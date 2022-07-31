#pragma once

#include "token_kind.h"

#include <stddef.h>

typedef struct PIdentifierInfo
{
  size_t hash;
  /* Either TOK_IDENTIFIER or a keyword token kind. */
  enum PTokenKind token_kind;
  /* The *real* length of the NUL-terminated string 'spelling'. Of course,
   * the final NUL character is not included in its length. */
  size_t spelling_len;
  char spelling[1];
} PIdentifierInfo;

struct PIdentifierTable
{
  struct PIdentifierInfo** identifiers;
  size_t bucket_count;
  size_t item_count;
  size_t rehashing_count;
};

/* Initializes the identifier table but do not add any identifier (even
 * keywords). Must be called before any other functions using p_table. */
void
p_identifier_table_init(struct PIdentifierTable* p_table);

/* Frees all memory allocated until now and destroys the identifier table.
 * All previously returned identifier infos are invalidated.
 * Using p_table after this function is undefined behavior. */
void
p_identifier_table_destroy(struct PIdentifierTable* p_table);

/* Dumps some stats about the identifier table for debugging purposes. */
void
p_identifier_table_dump_stats(struct PIdentifierTable* p_table);

/*
 *
 * If p_spelling_end is NULL, p_spelling_begin is assumed to be a
 * NUL-terminated string.
 */
struct PIdentifierInfo*
p_identifier_table_get(struct PIdentifierTable* p_table,
                       const char* p_spelling_begin,
                       const char* p_spelling_end);

/* Registers default keywords into the given identifier table. */
void
p_identifier_table_register_keywords(struct PIdentifierTable* p_table);
