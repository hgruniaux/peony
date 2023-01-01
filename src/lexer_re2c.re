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

                fill_token(p_token, P_TOK_COMMENT);
                p_token.data.literal.begin = m_marked_cursor;
                p_token.data.literal.end = m_cursor;
                break;
            }

            "/*" { goto block_comment; }

            // Identifiers:
            ident = [a-zA-Z_][a-zA-Z0-9_]*;
            ident { return fill_identifier_token(p_token, false); }
            "r#" ident { return fill_identifier_token(p_token, true); }

            // Integer literals:
            digit_sep = "_";
            int_suffix = [iu] ("8"|"16"|"32"|"64");

            bin_digit = "0"|"1";
            bin_literal = '0b' (bin_digit|digit_sep)* bin_digit (bin_digit|digit_sep)*;
            bin_literal int_suffix? { return fill_integer_literal(p_token, 2); }

            oct_digit = [0-7];
            oct_literal = '0o' (oct_digit|digit_sep)* oct_digit (oct_digit|digit_sep)*;
            oct_literal int_suffix? { return fill_integer_literal(p_token, 8); }

            dec_digit = [0-9];
            dec_literal = dec_digit (dec_digit|digit_sep)*;
            dec_literal int_suffix? { return fill_integer_literal(p_token, 10); }

            hex_digit = [0-9a-fA-F];
            hex_literal = '0x' (hex_digit|digit_sep)* hex_digit (hex_digit|digit_sep)*;
            hex_literal int_suffix? { return fill_integer_literal(p_token, 16); }

            // Floating-point literals:
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

            // String literals:
            quote_escape = "\\" ['"];
            ascii_escape = "\\" [nrt0\\] | "\\x" oct_digit hex_digit;
            unicode_escape = "\\u{" (hex_digit digit_sep*){1,6} "}";
            str_literal = "\"" ([^"\\\n\r\x00] | quote_escape | ascii_escape | unicode_escape)* "\"";
            str_literal { return fill_string_literal(p_token); }

            "{"   { return fill_token(p_token, P_TOK_LBRACE); }
            "}"   { return fill_token(p_token, P_TOK_RBRACE); }
            "("   { return fill_token(p_token, P_TOK_LPAREN); }
            ")"   { return fill_token(p_token, P_TOK_RPAREN); }
            "["   { return fill_token(p_token, P_TOK_LSQUARE); }
            "]"   { return fill_token(p_token, P_TOK_RSQUARE); }
            ","   { return fill_token(p_token, P_TOK_COMMA); }
            ":"   { return fill_token(p_token, P_TOK_COLON); }
            ";"   { return fill_token(p_token, P_TOK_SEMI); }
            "."   { return fill_token(p_token, P_TOK_DOT); }
            "->"  { return fill_token(p_token, P_TOK_ARROW); }
            "="   { return fill_token(p_token, P_TOK_EQUAL); }
            "=="  { return fill_token(p_token, P_TOK_EQUAL_EQUAL); }
            "<"   { return fill_token(p_token, P_TOK_LESS); }
            "<<"  { return fill_token(p_token, P_TOK_LESS_LESS); }
            "<="  { return fill_token(p_token, P_TOK_LESS_EQUAL); }
            "<<=" { return fill_token(p_token, P_TOK_LESS_LESS_EQUAL); }
            ">"   { return fill_token(p_token, P_TOK_GREATER); }
            ">>"  { return fill_token(p_token, P_TOK_GREATER_GREATER); }
            ">="  { return fill_token(p_token, P_TOK_GREATER_EQUAL); }
            ">>=" { return fill_token(p_token, P_TOK_GREATER_GREATER_EQUAL); }
            "!"   { return fill_token(p_token, P_TOK_EXCLAIM); }
            "!="  { return fill_token(p_token, P_TOK_EXCLAIM_EQUAL); }
            "+"   { return fill_token(p_token, P_TOK_PLUS); }
            "+="  { return fill_token(p_token, P_TOK_PLUS_EQUAL); }
            "-"   { return fill_token(p_token, P_TOK_MINUS); }
            "-="  { return fill_token(p_token, P_TOK_MINUS_EQUAL); }
            "*"   { return fill_token(p_token, P_TOK_STAR); }
            "*="  { return fill_token(p_token, P_TOK_STAR_EQUAL); }
            "/"   { return fill_token(p_token, P_TOK_SLASH); }
            "/="  { return fill_token(p_token, P_TOK_SLASH_EQUAL); }
            "%"   { return fill_token(p_token, P_TOK_PERCENT); }
            "%="  { return fill_token(p_token, P_TOK_PERCENT_EQUAL); }
            "^"   { return fill_token(p_token, P_TOK_CARET); }
            "^="  { return fill_token(p_token, P_TOK_CARET_EQUAL); }
            "|"   { return fill_token(p_token, P_TOK_PIPE); }
            "||"  { return fill_token(p_token, P_TOK_PIPE_PIPE); }
            "|="  { return fill_token(p_token, P_TOK_PIPE_EQUAL); }
            "&"   { return fill_token(p_token, P_TOK_AMP); }
            "&&"  { return fill_token(p_token, P_TOK_AMP_AMP); }
            "&="  { return fill_token(p_token, P_TOK_AMP_EQUAL); }

            "\x00" {
                fill_token(p_token, P_TOK_EOF);
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

            "*" "/" {
                if (!m_keep_comments)
                    continue;

                fill_token(p_token, P_TOK_COMMENT);
                p_token.data.literal.begin = m_marked_cursor;
                p_token.data.literal.end = m_cursor;
                break;
            }

            "\x00" {
                // Unterminated block comment
                fill_token(p_token, P_TOK_EOF);
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
