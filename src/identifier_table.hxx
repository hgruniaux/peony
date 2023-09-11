#ifndef PEONY_IDENTIFIER_TABLE_HXX
#define PEONY_IDENTIFIER_TABLE_HXX

#include "token_kind.hxx"
#include "utils/source_location.hxx"

#include <cstddef>

class PIdentifierInfo
{
public:
  // Internal identifier hash used by the identifier table implementation.
  // TODO: Should we really store the hash value or compute it again on the fly?
  size_t hash;
  // Either TOK_IDENTIFIER or a keyword token m_kind.
  PTokenKind token_kind;
  // The UTF-8 encoded spelling of the identifier (NUL-terminated) and its
  // byte-length (excluding the final NUL).
  size_t spelling_len;
  char spelling[1];
};

/// \brief A PIdentifierInfo with an attached PSourceRange.
struct PLocalizedIdentifierInfo
{
  PIdentifierInfo* ident;
  PSourceRange range;
};

/// \brief A dynamic hash table optimized for storing identifiers.
class PIdentifierTable
{
public:
  PIdentifierInfo** identifiers;
  size_t bucket_count;
  size_t item_count;
  size_t rehashing_count;
};

/// Initializes the identifier table but do not add any identifier (even
/// keywords). Must be called before any other functions using p_table.
void
p_identifier_table_init(PIdentifierTable* p_table);

/// Frees all memory allocated until now and destroys the identifier table.
/// All previously returned identifier infos are invalidated.
/// Using p_table after this function is undefined behavior.
void
p_identifier_table_destroy(PIdentifierTable* p_table);

/// Dumps some stats about the identifier table for debugging purposes.
void
p_identifier_table_dump_stats(PIdentifierTable* p_table);

/// If p_spelling_end is nullptr, p_spelling_begin is assumed to be a
/// NUL-terminated string.
PIdentifierInfo*
p_identifier_table_get(PIdentifierTable* p_table,
                       const char* p_spelling_begin,
                       const char* p_spelling_end);

/// Registers default Peony keywords into the given identifier table.
void
p_identifier_table_register_keywords(PIdentifierTable* p_table);

#endif // PEONY_IDENTIFIER_TABLE_HXX
