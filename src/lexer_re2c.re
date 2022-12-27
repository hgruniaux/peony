#include "lexer.hxx"

#include "utils/diag.hxx"
#include "options.hxx"

#include <cassert>
#include <cstdio>
#include <cstring>

/*!re2c
    re2c:yyfill:enable    = 0;
    re2c:encoding:utf8    = 0;
    re2c:indent:string    = "    ";
    re2c:define:YYCTYPE   = "char";
 */

void
PLexer::fill_token(PToken& p_token, PTokenKind p_kind)
{
    p_token.kind = p_kind;
    p_token.source_location = m_marked_source_location;
    p_token.token_length = m_cursor - m_marked_cursor;
}

void
PLexer::register_newline()
{
    uint32_t line_pos = m_cursor - source_file->get_buffer_raw();
    source_file->get_line_map().add(line_pos);
}

static void
parse_int_suffix(PToken& p_token)
{
    PIntLiteralSuffix suffix = P_ILS_NO_SUFFIX;

    if (p_token.data.literal.end - p_token.data.literal.begin >= 2) {
        const char* suffix_start = p_token.data.literal.end - 2;
        if (*suffix_start == 'i' || *suffix_start == 'u') {
            bool is_unsigned = *suffix_start == 'u';
            ++suffix_start;
            assert(*suffix_start == '8');
            p_token.data.literal.end -= 2;
            if (is_unsigned)
                suffix = P_ILS_U8;
            else
                suffix = P_ILS_I8;
        }
    }

    if (p_token.data.literal.end - p_token.data.literal.begin >= 3) {
        const char* suffix_start = p_token.data.literal.end - 3;
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

            p_token.data.literal.end -= 3;
        }
    }

    p_token.data.literal.suffix_kind = suffix;
}

void
PLexer::tokenize(PToken& p_token)
{
#define FILL_TOKEN(p_kind) (fill_token(p_token, p_kind))

    for (;;) {
        m_marked_cursor = m_cursor;
        m_marked_source_location = m_marked_cursor - source_file->get_buffer_raw();


        /*!re2c
            re2c:define:YYCURSOR   = "m_cursor";
            re2c:define:YYMARKER   = "m_marker";

            new_line = "\n"|"\r\n";
            new_line {
                register_newline();
                continue;
            }

            [ \t\v\f\r]+ { continue; }

            "//"[^\x00\n\r]* {
                if (!m_keep_comments)
                    continue;

                FILL_TOKEN(P_TOK_COMMENT);
                p_token.data.literal.begin = m_marked_cursor;
                p_token.data.literal.end = m_cursor;
                break;
            }

            "/*" { goto block_comment; }

            ident = [a-zA-Z_][a-zA-Z0-9_]*;
            ident {
                struct PIdentifierInfo* ident = p_identifier_table_get(
                    identifier_table,
                    m_marked_cursor,
                    m_cursor
                );

                assert(ident != nullptr);
                FILL_TOKEN(ident->token_kind);
                p_token.data.identifier = ident;
                break;
            }

            "r#" ident {
                struct PIdentifierInfo* ident = p_identifier_table_get(
                    identifier_table,
                    m_marked_cursor + 2,
                    m_cursor
                );

                assert(ident != nullptr);
                FILL_TOKEN(P_TOK_IDENTIFIER);
                p_token.data.identifier = ident;
                break;
            }

            digit_sep = "_";
            int_suffix = [iu] ("8"|"16"|"32"|"64");

            bin_digit = "0"|"1";
            bin_literal = '0b' (bin_digit|digit_sep)* bin_digit (bin_digit|digit_sep)*;
            bin_literal int_suffix? {
                FILL_TOKEN(P_TOK_INT_LITERAL);
                p_token.data.literal.int_radix = 2;
                p_token.data.literal.begin = m_marked_cursor;
                p_token.data.literal.end = m_cursor;
                parse_int_suffix(p_token);
                break;
            }

            oct_digit = [0-7];
            oct_literal = '0o' (oct_digit|digit_sep)* oct_digit (oct_digit|digit_sep)*;
            oct_literal int_suffix? {
                FILL_TOKEN(P_TOK_INT_LITERAL);
                p_token.data.literal.int_radix = 8;
                p_token.data.literal.begin = m_marked_cursor;
                p_token.data.literal.end = m_cursor;
                parse_int_suffix(p_token);
                break;
            }

            hex_digit = [0-9a-fA-F];
            hex_literal = '0x' (hex_digit|digit_sep)* hex_digit (hex_digit|digit_sep)*;
            hex_literal int_suffix? {
                FILL_TOKEN(P_TOK_INT_LITERAL);
                p_token.data.literal.int_radix = 16;
                p_token.data.literal.begin = m_marked_cursor;
                p_token.data.literal.end = m_cursor;
                parse_int_suffix(p_token);
                break;
            }

            dec_digit = [0-9];
            dec_literal = dec_digit (dec_digit|digit_sep)*;
            dec_literal int_suffix? {
                FILL_TOKEN(P_TOK_INT_LITERAL);
                p_token.data.literal.int_radix = 10;
                p_token.data.literal.begin = m_marked_cursor;
                p_token.data.literal.end = m_cursor;
                parse_int_suffix(p_token);
                break;
            }

            float_exp = 'e' [+-]? (dec_digit|digit_sep)* dec_digit (dec_digit|digit_sep)*;
            float_suffix = "f32" | "f64";
            dec_literal float_exp
            | dec_literal "." dec_literal float_exp?
            | dec_literal ("." dec_literal)? float_exp? float_suffix {
                FILL_TOKEN(P_TOK_FLOAT_LITERAL);
                p_token.data.literal.begin = m_marked_cursor;
                p_token.data.literal.end = m_cursor;

                PFloatLiteralSuffix suffix = P_FLS_NO_SUFFIX;
                if (p_token.data.literal.end - p_token.data.literal.begin >= 3) {
                    const char* suffix_start = p_token.data.literal.end - 3;
                    if (memcmp(suffix_start, "f32", 3) == 0) {
                        suffix = P_FLS_F32;
                        p_token.data.literal.end -= 3;
                    } else if (memcmp(suffix_start, "f64", 3) == 0) {
                        suffix = P_FLS_F64;
                        p_token.data.literal.end -= 3;
                    }
                }

                p_token.data.literal.suffix_kind = suffix;

                break;
            }

            "{"   { FILL_TOKEN(P_TOK_LBRACE); break; }
            "}"   { FILL_TOKEN(P_TOK_RBRACE); break; }
            "("   { FILL_TOKEN(P_TOK_LPAREN); break; }
            ")"   { FILL_TOKEN(P_TOK_RPAREN); break; }
            "["   { FILL_TOKEN(P_TOK_LSQUARE); break; }
            "]"   { FILL_TOKEN(P_TOK_RSQUARE); break; }
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
                p_token.token_length = 0;
                m_cursor--;
                break;
            }

            * {
                PDiag* d = diag_at(P_DK_err_unknown_character, m_marked_source_location);
                diag_add_arg_char(d, m_cursor[-1]);
                diag_flush(d);
                continue;
            }
        */

        block_comment:
        /*!re2c
            new_line {
                register_newline();
                goto block_comment;
            }

            "*/" {
                if (!m_keep_comments)
                    continue;

                FILL_TOKEN(P_TOK_COMMENT);
                p_token.data.literal.begin = m_marked_cursor;
                p_token.data.literal.end = m_cursor;
                break;
            }

            "\x00" {
                // Unterminated block comment
                FILL_TOKEN(P_TOK_EOF);
                m_cursor--;
                break;
            }

            * {
                goto block_comment;
            }
         */
    }

#undef FILL_TOKEN
}
