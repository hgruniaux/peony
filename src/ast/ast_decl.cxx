#include "ast_decl.hxx"
#include "ast_printer.hxx"

void
PDecl::dump(PContext& p_ctx)
{
  PAstPrinter printer(p_ctx);
  printer.visit(this);
}

PStructFieldDecl::PStructFieldDecl(PType* p_type, PLocalizedIdentifierInfo p_localized_name, PSourceRange p_src_range)
  : PDecl(DECL_KIND, p_type, p_localized_name, p_src_range)
  , m_parent(nullptr)
  , m_index_in_parent_fields(0)
{
}

PStructDecl::PStructDecl(PContext& p_ctx,
                         PLocalizedIdentifierInfo p_name,
                         PArrayView<PStructFieldDecl*> p_fields,
                         PSourceRange p_src_range)
  : PDecl(DECL_KIND, nullptr, p_name, p_src_range)
  , m_fields(p_fields)
{
  m_type = p_ctx.get_tag_ty(this);

  size_t i = 0;
  for (auto* field : p_fields) {
    field->m_parent = this;
    field->m_index_in_parent_fields = i++;
  }
}

PStructFieldDecl*
PStructDecl::find_field(PIdentifierInfo* p_name) const
{
  auto it = std::find_if(
    m_fields.begin(), m_fields.end(), [p_name](PStructFieldDecl* p_field) { return p_field->get_name() == p_name; });
  if (it == m_fields.end())
    return nullptr;
  return *it;
}
