#ifndef PEONY_SCOPE_HXX
#define PEONY_SCOPE_HXX

#include "identifier_table.hxx"

#include <unordered_map>

class PDecl;
class PAst;

struct PScope;

struct PSymbol
{
  PScope* scope;
  PIdentifierInfo* name;
  PDecl* decl;

  PSymbol(PScope* p_scope, PIdentifierInfo* p_name)
    : scope(p_scope)
    , name(p_name)
    , decl(nullptr)
  {
  }
};

enum PScopeFlags
{
  P_SF_NONE = 0x00,
  P_SF_BREAK = 0x01,       /* Scope can contain `break` statements. */
  P_SF_CONTINUE = 0x02,    /* Scope can contain `continue` statements. */
  P_SF_FUNC_PARAMS = 0x04, /* Scope where the parameters of a function are placed. */
};

struct PScope
{
  PScope* parent_scope;
  PAst* statement; /* statement at origin of this scope (PAstWhileStmt, PAstCompoundStmt, etc.) */
  std::unordered_map<PIdentifierInfo*, PSymbol*> symbols;
  PScopeFlags flags;

  PScope(PScope* p_parent_scope, PScopeFlags p_flags = P_SF_NONE);
};

PSymbol*
p_scope_local_lookup(PScope* p_scope, PIdentifierInfo* p_name);

PSymbol*
p_scope_add_symbol(PScope* p_scope, PIdentifierInfo* p_name);

#endif // PEONY_SCOPE_HXX
