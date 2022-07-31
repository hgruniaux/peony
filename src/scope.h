#pragma once

#include "identifier_table.h"

typedef struct PScope PScope;

typedef struct PSymbol
{
  PScope* scope;
  PIdentifierInfo* name;
  struct PDecl* decl;
} PSymbol;

typedef enum PScopeFlags
{
  P_SF_NONE = 0x00,
  P_SF_BREAK = 0x01, /* Scope can contains `break` statements. */
  P_SF_CONTINUE = 0x02, /* Scope can contains `continue` statements. */
} PScopeFlags;

struct PScope
{
  PScope* parent_scope;
  struct PAst* statement; /* statement at origin of this scope (PAstWhileStmt, PAstCompoundStmt, etc.) */
  PSymbol** symbols;
  PScopeFlags flags;
  size_t bucket_count;
  size_t item_count;
};

void
p_scope_init(PScope* p_scope, PScope* p_parent_scope, PScopeFlags p_flags);

void
p_scope_destroy(PScope* p_scope);

void
p_scope_clear(PScope* p_scope);

PSymbol*
p_scope_local_lookup(PScope* p_scope, PIdentifierInfo* p_name);

PSymbol*
p_scope_add_symbol(PScope* p_scope, PIdentifierInfo* p_name);
