#include "lexer.h"

#include "utils/diag.h"

#include <assert.h>
#include <stdio.h>

/*!re2c
    re2c:yyfill:enable    = 0;
    re2c:encoding:utf8    = 0;
    re2c:indent:string    = "    ";
    re2c:define:YYCTYPE   = "char";
 */

void
p_lex(struct PLexer* p_lexer, struct PToken* p_token)
{
    assert(p_lexer != NULL && p_token != NULL);

    for (;;) {
        const char* tok_begin = p_lexer->cursor;

        /*!re2c
            re2c:define:YYCURSOR   = "p_lexer->cursor";
            re2c:define:YYMARKER   = "p_lexer->marker";

            "\n"|"\r\n"{
                ++p_lexer->lineno;
                ++CURRENT_LINENO;
                continue;
            }

            [ \t\v\f\r]+ { continue; }

            "#"[^\x00\n\r]* {
                if (!p_lexer->b_keep_comments)
                    continue;

                p_token->kind = P_TOK_COMMENT;
                break;
            }

            [a-zA-Z_][a-zA-Z0-9_]* {
                struct PIdentifierInfo* ident = p_identifier_table_get(
                    p_lexer->identifier_table,
                    tok_begin,
                    p_lexer->cursor
                );

                assert(ident != NULL);
                p_token->kind = ident->token_kind;
                p_token->data.identifier_info = ident;
                break;
            }

            [0-9]+ {
                p_token->kind = P_TOK_INT_LITERAL;
                p_token->data.literal_begin = tok_begin;
                p_token->data.literal_end = p_lexer->cursor;
                break;
            }

            "{"   { p_token->kind = P_TOK_LBRACE; break; }
            "}"   { p_token->kind = P_TOK_RBRACE; break; }
            "("   { p_token->kind = P_TOK_LPAREN; break; }
            ")"   { p_token->kind = P_TOK_RPAREN; break; }
            ","   { p_token->kind = P_TOK_COMMA; break; }
            ":"   { p_token->kind = P_TOK_COLON; break; }
            ";"   { p_token->kind = P_TOK_SEMI; break; }
            "="   { p_token->kind = P_TOK_EQUAL; break; }
            "=="  { p_token->kind = P_TOK_EQUAL_EQUAL; break; }
            "<"   { p_token->kind = P_TOK_LESS; break; }
            "<<"  { p_token->kind = P_TOK_LESS_LESS; break; }
            "<="  { p_token->kind = P_TOK_LESS_EQUAL; break; }
            "<<=" { p_token->kind = P_TOK_LESS_LESS_EQUAL; break; }
            ">"   { p_token->kind = P_TOK_GREATER; break; }
            ">>"  { p_token->kind = P_TOK_GREATER_GREATER; break; }
            ">="  { p_token->kind = P_TOK_GREATER_EQUAL; break; }
            ">>=" { p_token->kind = P_TOK_GREATER_GREATER_EQUAL; break; }
            "!"   { p_token->kind = P_TOK_EXCLAIM; break; }
            "!="  { p_token->kind = P_TOK_EXCLAIM_EQUAL; break; }
            "+"   { p_token->kind = P_TOK_PLUS; break; }
            "+="  { p_token->kind = P_TOK_PLUS_EQUAL; break; }
            "-"   { p_token->kind = P_TOK_MINUS; break; }
            "-="  { p_token->kind = P_TOK_MINUS_EQUAL; break; }
            "*"   { p_token->kind = P_TOK_STAR; break; }
            "*="  { p_token->kind = P_TOK_STAR_EQUAL; break; }
            "/"   { p_token->kind = P_TOK_SLASH; break; }
            "/="  { p_token->kind = P_TOK_SLASH_EQUAL; break; }
            "%"   { p_token->kind = P_TOK_PERCENT; break; }
            "%="  { p_token->kind = P_TOK_PERCENT_EQUAL; break; }
            "^"   { p_token->kind = P_TOK_CARET; break; }
            "^="  { p_token->kind = P_TOK_CARET_EQUAL; break; }
            "|"   { p_token->kind = P_TOK_PIPE; break; }
            "||"  { p_token->kind = P_TOK_PIPE_PIPE; break; }
            "|="  { p_token->kind = P_TOK_PIPE_EQUAL; break; }
            "&"   { p_token->kind = P_TOK_AMP; break; }
            "&&"  { p_token->kind = P_TOK_AMP_AMP; break; }
            "&="  { p_token->kind = P_TOK_AMP_EQUAL; break; }

            "\x00" {
                p_token->kind = P_TOK_EOF;
                break;
            }

            * {
                error("unknown character '%c'\n", p_lexer->cursor[-1]);
                continue;
            }
        */
    }
}
