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

static void
fill_token(PLexer* p_lexer, PToken* p_token, PTokenKind p_kind)
{
    p_token->kind = p_kind;
    p_token->source_location = p_lexer->marked_source_location;
    p_token->token_length = p_lexer->cursor - p_lexer->marked_cursor;
}

void
p_lex(PLexer* p_lexer, PToken* p_token)
{
    assert(p_lexer != NULL && p_token != NULL);

    for (;;) {
        p_lexer->marked_cursor = p_lexer->cursor;
        p_lexer->marked_source_location = p_lexer->marked_cursor - p_lexer->source_file->buffer;

        #define FILL_TOKEN(p_kind) (fill_token(p_lexer, p_token, p_kind))

        /*!re2c
            re2c:define:YYCURSOR   = "p_lexer->cursor";
            re2c:define:YYMARKER   = "p_lexer->marker";

            "\n"|"\r\n" {
                ++CURRENT_LINENO;

                uint32_t line_pos = p_lexer->cursor - p_lexer->source_file->buffer;
                p_line_map_add(&p_lexer->source_file->line_map, line_pos);
                continue;
            }

            [ \t\v\f\r]+ { continue; }

            "#"[^\x00\n\r]* {
                if (!g_verify_mode_enabled)
                    continue;

                FILL_TOKEN(P_TOK_COMMENT);
                p_token->data.literal_begin = p_lexer->marked_cursor + 1;
                p_token->data.literal_end = p_lexer->cursor;
                break;
            }

            [a-zA-Z_][a-zA-Z0-9_]* {
                struct PIdentifierInfo* ident = p_identifier_table_get(
                    p_lexer->identifier_table,
                    p_lexer->marked_cursor,
                    p_lexer->cursor
                );

                assert(ident != NULL);
                FILL_TOKEN(ident->token_kind);
                p_token->data.identifier_info = ident;
                break;
            }

            [0-9]+ {
                FILL_TOKEN(P_TOK_INT_LITERAL);
                p_token->data.literal_begin = p_lexer->marked_cursor;
                p_token->data.literal_end = p_lexer->cursor;
                break;
            }

            "{"   { FILL_TOKEN(P_TOK_LBRACE); break; }
            "}"   { FILL_TOKEN(P_TOK_RBRACE); break; }
            "("   { FILL_TOKEN(P_TOK_LPAREN); break; }
            ")"   { FILL_TOKEN(P_TOK_RPAREN); break; }
            ","   { FILL_TOKEN(P_TOK_COMMA); break; }
            ":"   { FILL_TOKEN(P_TOK_COLON); break; }
            ";"   { FILL_TOKEN(P_TOK_SEMI); break; }
            "->"  { FILL_TOKEN(P_TOK_ARROW); break; }
            "="   { FILL_TOKEN(P_TOK_EQUAL); break; }
            "=="  { FILL_TOKEN(P_TOK_EQUAL_EQUAL); break; }
            "<"   { FILL_TOKEN(P_TOK_LESS); break; }
            "<<"  { FILL_TOKEN(P_TOK_LESS_LESS); break; }
            "<="  { FILL_TOKEN(P_TOK_LESS_EQUAL); break; }
            "<<=" { FILL_TOKEN(P_TOK_LESS_LESS_EQUAL); break; }
            ">"   { FILL_TOKEN(P_TOK_GREATER); break; }
            ">>"  { FILL_TOKEN(P_TOK_GREATER_GREATER); break; }
            ">="  { FILL_TOKEN(P_TOK_GREATER_EQUAL); break; }
            ">>=" { FILL_TOKEN(P_TOK_GREATER_GREATER_EQUAL); break; }
            "!"   { FILL_TOKEN(P_TOK_EXCLAIM); break; }
            "!="  { FILL_TOKEN(P_TOK_EXCLAIM_EQUAL); break; }
            "+"   { FILL_TOKEN(P_TOK_PLUS); break; }
            "+="  { FILL_TOKEN(P_TOK_PLUS_EQUAL); break; }
            "-"   { FILL_TOKEN(P_TOK_MINUS); break; }
            "-="  { FILL_TOKEN(P_TOK_MINUS_EQUAL); break; }
            "*"   { FILL_TOKEN(P_TOK_STAR); break; }
            "*="  { FILL_TOKEN(P_TOK_STAR_EQUAL); break; }
            "/"   { FILL_TOKEN(P_TOK_SLASH); break; }
            "/="  { FILL_TOKEN(P_TOK_SLASH_EQUAL); break; }
            "%"   { FILL_TOKEN(P_TOK_PERCENT); break; }
            "%="  { FILL_TOKEN(P_TOK_PERCENT_EQUAL); break; }
            "^"   { FILL_TOKEN(P_TOK_CARET); break; }
            "^="  { FILL_TOKEN(P_TOK_CARET_EQUAL); break; }
            "|"   { FILL_TOKEN(P_TOK_PIPE); break; }
            "||"  { FILL_TOKEN(P_TOK_PIPE_PIPE); break; }
            "|="  { FILL_TOKEN(P_TOK_PIPE_EQUAL); break; }
            "&"   { FILL_TOKEN(P_TOK_AMP); break; }
            "&&"  { FILL_TOKEN(P_TOK_AMP_AMP); break; }
            "&="  { FILL_TOKEN(P_TOK_AMP_EQUAL); break; }

            "\x00" {
                FILL_TOKEN(P_TOK_EOF);
                break;
            }

            * {
                error("unknown character '%c'\n", p_lexer->cursor[-1]);
                continue;
            }
        */
    }
}
