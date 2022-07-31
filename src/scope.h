#pragma once

#include "identifier_table.h"

typedef struct PScope PScope;

typedef struct PSymbol
{
  PScope* scope;
  PIdentifierInfo* name;
  struct PDecl* decl;
} PSymbol;

struct PScope
{
  PScope* parent_scope;
  PSymbol** symbols;
  size_t bucket_count;
  size_t item_count;
};

void
p_scope_init(PScope* p_scope, PScope* p_parent_scope);

void
p_scope_destroy(PScope* p_scope);

void
p_scope_clear(PScope* p_scope);

PSymbol*
p_scope_local_lookup(PScope* p_scope, struct PIdentifierInfo* p_name);

PSymbol*
p_scope_add_symbol(PScope* p_scope, struct PIdentifierInfo* p_name);
