#ifndef PEONY_IDENTIFIER_TABLE_HXX
#define PEONY_IDENTIFIER_TABLE_HXX

#include "token_kind.hxx"
#include "utils/bump_allocator.hxx"
#include "utils/source_location.hxx"

#include <cstddef>
#include <unordered_map>

/// \brief Represents an identifier or a keyword in the source code.
///
/// Two semantically equivalent identifiers are stored at the same address.
/// Therefore, comparing if two identifiers are equivalent is the same as
/// doing a pointer identity test.
class PIdentifierInfo
{
public:
  /// Returns the UTF-8 encoded spelling of the identifier.
  [[nodiscard]] std::string_view get_spelling() const { return { m_spelling, m_spelling_len }; }

  /// Returns the token kind associated to this identifier. Either IDENTIFIER or a keyword.
  [[nodiscard]] PTokenKind get_token_kind() const { return m_token_kind; }
  void set_token_kind(PTokenKind kind) { m_token_kind = kind; }

private:
  friend class PIdentifierTable;
  PTokenKind m_token_kind;
  size_t m_spelling_len;
  char m_spelling[1];
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
  [[nodiscard]] PIdentifierInfo* get(const char* p_spelling_begin, const char* p_spelling_end);
  [[nodiscard]] PIdentifierInfo* get(std::string_view p_spelling);

  void register_keywords();

private:
  PBumpAllocator m_allocator;
  std::unordered_map<std::string_view, PIdentifierInfo*> m_mapping;
};

#endif // PEONY_IDENTIFIER_TABLE_HXX
