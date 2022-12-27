#ifndef PEONY_AST_HXX
#define PEONY_AST_HXX

#include "ast/ast_decl.hxx"
#include "ast/ast_expr.hxx"
#include "ast/ast_stmt.hxx"

#define P_DECL_GET_NAME(p_node) (((PDecl*)(p_node))->name)
#define P_DECL_GET_TYPE(p_node) (((PDecl*)(p_node))->type)

#endif // PEONY_AST_HXX
