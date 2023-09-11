#include "identifier_table.hxx"

#include <cstring>

PIdentifierInfo*
PIdentifierTable::get(const char* p_spelling_begin, const char* p_spelling_end)
{
  size_t length;
  if (p_spelling_end == nullptr)
    length = strlen(p_spelling_begin);
  else
    length = std::distance(p_spelling_begin, p_spelling_end);

  return get({ p_spelling_begin, length });
}

PIdentifierInfo*
PIdentifierTable::get(std::string_view p_spelling)
{
  const auto it = m_mapping.find(p_spelling);
  if (it != m_mapping.end())
    return it->second;

  auto* identifier = m_allocator.alloc_with_extra_size<PIdentifierInfo>(sizeof(char) * p_spelling.size());
  identifier->set_token_kind(P_TOK_IDENTIFIER);
  identifier->m_spelling_len = p_spelling.size();
  memcpy(identifier->m_spelling, p_spelling.data(), sizeof(char) * p_spelling.size());
  identifier->m_spelling[p_spelling.size()] = '\0';

  m_mapping.insert({ identifier->get_spelling(), identifier });
  return identifier;
}

void
PIdentifierTable::register_keywords()
{
#define TOKEN(p_kind)
#define KEYWORD(p_spelling) get(#p_spelling)->set_token_kind(P_TOK_KEY_##p_spelling);
#include "token_kind.def"
}
