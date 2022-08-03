#include "lexer.h"

#include "utils/diag.h"
#include "options.h"

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

static void
register_new_line(PLexer* p_lexer)
{
    uint32_t line_pos = p_lexer->cursor - p_lexer->source_file->buffer;
    p_line_map_add(&p_lexer->source_file->line_map, line_pos);
}

static void
parse_int_suffix(PToken* p_token)
{
    PIntLiteralSuffix suffix = P_ILS_NO_SUFFIX;

    if (p_token->data.literal.end - p_token->data.literal.begin >= 2) {
        const char* suffix_start = p_token->data.literal.end - 2;
        if (*suffix_start == 'i' || *suffix_start == 'u') {
            bool is_unsigned = *suffix_start == 'u';
            ++suffix_start;
            assert(*suffix_start == '8');
            p_token->data.literal.end -= 2;
            if (is_unsigned)
                suffix = P_ILS_U8;
            else
                suffix = P_ILS_I8;
        }
    }

    if (p_token->data.literal.end - p_token->data.literal.begin >= 3) {
        const char* suffix_start = p_token->data.literal.end - 3;
        if (*suffix_start == 'i' || *suffix_start == 'u') {
            bool is_unsigned = *suffix_start == 'u';
            ++suffix_start;
            assert(*suffix_start == '1' || *suffix_start == '3' || *suffix_start == '6');
            ++suffix_start;

            if (*suffix_start == '2') {
                if (is_unsigned)
                    suffix = P_ILS_U32;
                else
                    suffix = P_ILS_I32;
            } else if (*suffix_start == '4') {
                if (is_unsigned)
                    suffix = P_ILS_U64;
                else
                    suffix = P_ILS_I64;
            } else { // *suffix_start == '6'
                if (is_unsigned)
                    suffix = P_ILS_U16;
                else
                    suffix = P_ILS_I16;
            }

            p_token->data.literal.end -= 3;
        }
    }

    p_token->data.literal.suffix_kind = suffix;
}

void
lexer_next(PLexer* p_lexer, PToken* p_token)
{
    assert(p_lexer != NULL && p_token != NULL);

#define FILL_TOKEN(p_kind) (fill_token(p_lexer, p_token, p_kind))

    for (;;) {
        p_lexer->marked_cursor = p_lexer->cursor;
        p_lexer->marked_source_location = p_lexer->marked_cursor - p_lexer->source_file->buffer;
        g_current_source_location = p_lexer->marked_source_location;


        /*!re2c
            re2c:define:YYCURSOR   = "p_lexer->cursor";
            re2c:define:YYMARKER   = "p_lexer->marker";

            new_line = "\n"|"\r\n";
            new_line {
                register_new_line(p_lexer);
                continue;
            }

            [ \t\v\f\r]+ { continue; }

            "//"[^\x00\n\r]* {
                continue;

                #if 0
                FILL_TOKEN(P_TOK_COMMENT);
                p_token->data.literal.begin = p_lexer->marked_cursor;
                p_token->data.literal.end = p_lexer->cursor;
                break;
                #endif
            }

            "/*" { goto block_comment; }

            ident = [a-zA-Z_][a-zA-Z0-9_]*;
            ident {
                struct PIdentifierInfo* ident = p_identifier_table_get(
                    p_lexer->identifier_table,
                    p_lexer->marked_cursor,
                    p_lexer->cursor
                );

                assert(ident != NULL);
                FILL_TOKEN(ident->token_kind);
                p_token->data.identifier = ident;
                break;
            }

            "r#" ident {
                struct PIdentifierInfo* ident = p_identifier_table_get(
                    p_lexer->identifier_table,
                    p_lexer->marked_cursor + 2,
                    p_lexer->cursor
                );

                assert(ident != NULL);
                FILL_TOKEN(P_TOK_IDENTIFIER);
                p_token->data.identifier = ident;
                break;
            }

            int_suffix = [iu] ("8"|"16"|"32"|"64");

            bin_digit = "0"|"1";
            '0b' (bin_digit|"_")* bin_digit (bin_digit|"_")* int_suffix? {
                FILL_TOKEN(P_TOK_INT_LITERAL);
                p_token->data.literal.int_radix = 2;
                p_token->data.literal.begin = p_lexer->marked_cursor;
                p_token->data.literal.end = p_lexer->cursor;
                parse_int_suffix(p_token);
                break;
            }

            hex_digit = [0-9a-fA-F];
            '0x' (hex_digit|"_")* hex_digit (hex_digit|"_")* int_suffix? {
                FILL_TOKEN(P_TOK_INT_LITERAL);
                p_token->data.literal.int_radix = 16;
                p_token->data.literal.begin = p_lexer->marked_cursor;
                p_token->data.literal.end = p_lexer->cursor;
                parse_int_suffix(p_token);
                break;
            }

            dec_digit = [0-9];
            dec_literal = dec_digit (dec_digit|"_")*;
            dec_literal int_suffix? {
                FILL_TOKEN(P_TOK_INT_LITERAL);
                p_token->data.literal.int_radix = 10;
                p_token->data.literal.begin = p_lexer->marked_cursor;
                p_token->data.literal.end = p_lexer->cursor;
                parse_int_suffix(p_token);
                break;
            }

            float_exp = 'e' [+-]? (dec_digit|"_")* dec_digit (dec_digit|"_")*;
            float_suffix = "f32" | "f64";
            dec_literal float_exp
            | dec_literal "." dec_literal float_exp?
            | dec_literal ("." dec_literal)? float_exp? float_suffix {
                FILL_TOKEN(P_TOK_FLOAT_LITERAL);
                p_token->data.literal.begin = p_lexer->marked_cursor;
                p_token->data.literal.end = p_lexer->cursor;

                PFloatLiteralSuffix suffix = P_FLS_NO_SUFFIX;
                if (p_token->data.literal.end - p_token->data.literal.begin >= 3) {
                    const char* suffix_start = p_token->data.literal.end - 3;
                    if (memcmp(suffix_start, "f32", 3) == 0) {
                        suffix = P_FLS_F32;
                        p_token->data.literal.end -= 3;
                    } else if (memcmp(suffix_start, "f64", 3) == 0) {
                        suffix = P_FLS_F64;
                        p_token->data.literal.end -= 3;
                    }
                }

                p_token->data.literal.suffix_kind = suffix;

                break;
            }

            "{"   { FILL_TOKEN(P_TOK_LBRACE); break; }
            "}"   { FILL_TOKEN(P_TOK_RBRACE); break; }
            "("   { FILL_TOKEN(P_TOK_LPAREN); break; }
            ")"   { FILL_TOKEN(P_TOK_RPAREN); break; }
            ","   { FILL_TOKEN(P_TOK_COMMA); break; }
            ":"   { FILL_TOKEN(P_TOK_COLON); break; }
            ";"   { FILL_TOKEN(P_TOK_SEMI); break; }
            "."   { FILL_TOKEN(P_TOK_DOT); break; }
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
                p_lexer->cursor--;
                break;
            }

            * {
                PDiag* d = diag_at(P_DK_err_unknown_character, p_lexer->marked_source_location);
                diag_add_arg_char(d, p_lexer->cursor[-1]);
                diag_flush(d);
                continue;
            }
        */

        block_comment:
        /*!re2c
            new_line {
                register_new_line(p_lexer);
                goto block_comment;
            }

            "*/" {
                continue;

                #if 0
                FILL_TOKEN(P_TOK_COMMENT);
                p_token->data.literal.begin = p_lexer->marked_cursor;
                p_token->data.literal.end = p_lexer->cursor;
                break;
                #endif
            }

            "\x00" {
                // Unterminated block comment
                FILL_TOKEN(P_TOK_EOF);
                p_lexer->cursor--;
                break;
            }

            * {
                goto block_comment;
            }
         */
    }

#undef FILL_TOKEN
}
