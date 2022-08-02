/* Generated by re2c */
#include "lexer.h"

#include "utils/diag.h"
#include "options.h"

#include <assert.h>
#include <stdio.h>



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
        const char* suffix_start = p_token->data.literal.end - 2;
        if (*suffix_start == 'i' || *suffix_start == 'u') {
            bool is_unsigned = *suffix_start == 'u';
            ++suffix_start;
            assert(*suffix_start == '3' || *suffix_start == '6');
            ++suffix_start;

            if (*suffix_start == '2') {
                if (is_unsigned)
                    suffix = P_ILS_U32;
                else
                    suffix = P_ILS_I32;
            } else { // *suffix_start == '4'
                if (is_unsigned)
                    suffix = P_ILS_U64;
                else
                    suffix = P_ILS_I64;
            }

            p_token->data.literal.end -= 3;
        }
    }

    p_token->data.literal.suffix_kind = suffix;
}

void
p_lex(PLexer* p_lexer, PToken* p_token)
{
    assert(p_lexer != NULL && p_token != NULL);

#define FILL_TOKEN(p_kind) (fill_token(p_lexer, p_token, p_kind))

    for (;;) {
        p_lexer->marked_cursor = p_lexer->cursor;
        p_lexer->marked_source_location = p_lexer->marked_cursor - p_lexer->source_file->buffer;
        g_current_source_location = p_lexer->marked_source_location;


        
{
    char yych;
    unsigned int yyaccept = 0;
    yych = *p_lexer->cursor;
    switch (yych) {
        case 0x00: goto yy1;
        case '\t':
        case '\v':
        case '\f':
        case ' ': goto yy3;
        case '\n': goto yy6;
        case '\r': goto yy7;
        case '!': goto yy8;
        case '%': goto yy10;
        case '&': goto yy12;
        case '(': goto yy14;
        case ')': goto yy15;
        case '*': goto yy16;
        case '+': goto yy18;
        case ',': goto yy20;
        case '-': goto yy21;
        case '.': goto yy23;
        case '/': goto yy24;
        case '0': goto yy26;
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9': goto yy28;
        case ':': goto yy30;
        case ';': goto yy31;
        case '<': goto yy32;
        case '=': goto yy34;
        case '>': goto yy36;
        case 'A':
        case 'B':
        case 'C':
        case 'D':
        case 'E':
        case 'F':
        case 'G':
        case 'H':
        case 'I':
        case 'J':
        case 'K':
        case 'L':
        case 'M':
        case 'N':
        case 'O':
        case 'P':
        case 'Q':
        case 'R':
        case 'S':
        case 'T':
        case 'U':
        case 'V':
        case 'W':
        case 'X':
        case 'Y':
        case 'Z':
        case '_':
        case 'a':
        case 'b':
        case 'c':
        case 'd':
        case 'e':
        case 'f':
        case 'g':
        case 'h':
        case 'i':
        case 'j':
        case 'k':
        case 'l':
        case 'm':
        case 'n':
        case 'o':
        case 'p':
        case 'q':
        case 'r':
        case 's':
        case 't':
        case 'u':
        case 'v':
        case 'w':
        case 'x':
        case 'y':
        case 'z': goto yy38;
        case '^': goto yy40;
        case '{': goto yy42;
        case '|': goto yy43;
        case '}': goto yy45;
        default: goto yy2;
    }
yy1:
    ++p_lexer->cursor;
    {
                FILL_TOKEN(P_TOK_EOF);
                p_lexer->cursor--;
                break;
            }
yy2:
    ++p_lexer->cursor;
    {
                PDiag* d = diag_at(P_DK_err_unknown_character, p_lexer->marked_source_location);
                diag_add_arg_char(d, p_lexer->cursor[-1]);
                diag_flush(d);
                continue;
            }
yy3:
    yych = *++p_lexer->cursor;
yy4:
    switch (yych) {
        case '\t':
        case '\v':
        case '\f':
        case '\r':
        case ' ': goto yy3;
        default: goto yy5;
    }
yy5:
    { continue; }
yy6:
    ++p_lexer->cursor;
    {
                register_new_line(p_lexer);
                continue;
            }
yy7:
    yych = *++p_lexer->cursor;
    switch (yych) {
        case '\n': goto yy6;
        default: goto yy4;
    }
yy8:
    yych = *++p_lexer->cursor;
    switch (yych) {
        case '=': goto yy46;
        default: goto yy9;
    }
yy9:
    { FILL_TOKEN(P_TOK_EXCLAIM); break; }
yy10:
    yych = *++p_lexer->cursor;
    switch (yych) {
        case '=': goto yy47;
        default: goto yy11;
    }
yy11:
    { FILL_TOKEN(P_TOK_PERCENT); break; }
yy12:
    yych = *++p_lexer->cursor;
    switch (yych) {
        case '&': goto yy48;
        case '=': goto yy49;
        default: goto yy13;
    }
yy13:
    { FILL_TOKEN(P_TOK_AMP); break; }
yy14:
    ++p_lexer->cursor;
    { FILL_TOKEN(P_TOK_LPAREN); break; }
yy15:
    ++p_lexer->cursor;
    { FILL_TOKEN(P_TOK_RPAREN); break; }
yy16:
    yych = *++p_lexer->cursor;
    switch (yych) {
        case '=': goto yy50;
        default: goto yy17;
    }
yy17:
    { FILL_TOKEN(P_TOK_STAR); break; }
yy18:
    yych = *++p_lexer->cursor;
    switch (yych) {
        case '=': goto yy51;
        default: goto yy19;
    }
yy19:
    { FILL_TOKEN(P_TOK_PLUS); break; }
yy20:
    ++p_lexer->cursor;
    { FILL_TOKEN(P_TOK_COMMA); break; }
yy21:
    yych = *++p_lexer->cursor;
    switch (yych) {
        case '=': goto yy52;
        case '>': goto yy53;
        default: goto yy22;
    }
yy22:
    { FILL_TOKEN(P_TOK_MINUS); break; }
yy23:
    ++p_lexer->cursor;
    { FILL_TOKEN(P_TOK_DOT); break; }
yy24:
    yych = *++p_lexer->cursor;
    switch (yych) {
        case '*': goto yy54;
        case '/': goto yy55;
        case '=': goto yy57;
        default: goto yy25;
    }
yy25:
    { FILL_TOKEN(P_TOK_SLASH); break; }
yy26:
    yyaccept = 0;
    yych = *(p_lexer->marker = ++p_lexer->cursor);
    switch (yych) {
        case 'B':
        case 'b': goto yy60;
        case 'X':
        case 'x': goto yy62;
        default: goto yy29;
    }
yy27:
    {
                FILL_TOKEN(P_TOK_INT_LITERAL);
                p_token->data.literal.int_radix = 10;
                p_token->data.literal.begin = p_lexer->marked_cursor;
                p_token->data.literal.end = p_lexer->cursor;
                parse_int_suffix(p_token);
                break;
            }
yy28:
    yyaccept = 0;
    yych = *(p_lexer->marker = ++p_lexer->cursor);
yy29:
    switch (yych) {
        case '.': goto yy58;
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        case '_': goto yy28;
        case 'E':
        case 'e': goto yy61;
        case 'f': goto yy63;
        case 'i':
        case 'u': goto yy64;
        default: goto yy27;
    }
yy30:
    ++p_lexer->cursor;
    { FILL_TOKEN(P_TOK_COLON); break; }
yy31:
    ++p_lexer->cursor;
    { FILL_TOKEN(P_TOK_SEMI); break; }
yy32:
    yych = *++p_lexer->cursor;
    switch (yych) {
        case '<': goto yy65;
        case '=': goto yy67;
        default: goto yy33;
    }
yy33:
    { FILL_TOKEN(P_TOK_LESS); break; }
yy34:
    yych = *++p_lexer->cursor;
    switch (yych) {
        case '=': goto yy68;
        default: goto yy35;
    }
yy35:
    { FILL_TOKEN(P_TOK_EQUAL); break; }
yy36:
    yych = *++p_lexer->cursor;
    switch (yych) {
        case '=': goto yy69;
        case '>': goto yy70;
        default: goto yy37;
    }
yy37:
    { FILL_TOKEN(P_TOK_GREATER); break; }
yy38:
    yych = *++p_lexer->cursor;
    switch (yych) {
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        case 'A':
        case 'B':
        case 'C':
        case 'D':
        case 'E':
        case 'F':
        case 'G':
        case 'H':
        case 'I':
        case 'J':
        case 'K':
        case 'L':
        case 'M':
        case 'N':
        case 'O':
        case 'P':
        case 'Q':
        case 'R':
        case 'S':
        case 'T':
        case 'U':
        case 'V':
        case 'W':
        case 'X':
        case 'Y':
        case 'Z':
        case '_':
        case 'a':
        case 'b':
        case 'c':
        case 'd':
        case 'e':
        case 'f':
        case 'g':
        case 'h':
        case 'i':
        case 'j':
        case 'k':
        case 'l':
        case 'm':
        case 'n':
        case 'o':
        case 'p':
        case 'q':
        case 'r':
        case 's':
        case 't':
        case 'u':
        case 'v':
        case 'w':
        case 'x':
        case 'y':
        case 'z': goto yy38;
        default: goto yy39;
    }
yy39:
    {
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
yy40:
    yych = *++p_lexer->cursor;
    switch (yych) {
        case '=': goto yy72;
        default: goto yy41;
    }
yy41:
    { FILL_TOKEN(P_TOK_CARET); break; }
yy42:
    ++p_lexer->cursor;
    { FILL_TOKEN(P_TOK_LBRACE); break; }
yy43:
    yych = *++p_lexer->cursor;
    switch (yych) {
        case '=': goto yy73;
        case '|': goto yy74;
        default: goto yy44;
    }
yy44:
    { FILL_TOKEN(P_TOK_PIPE); break; }
yy45:
    ++p_lexer->cursor;
    { FILL_TOKEN(P_TOK_RBRACE); break; }
yy46:
    ++p_lexer->cursor;
    { FILL_TOKEN(P_TOK_EXCLAIM_EQUAL); break; }
yy47:
    ++p_lexer->cursor;
    { FILL_TOKEN(P_TOK_PERCENT_EQUAL); break; }
yy48:
    ++p_lexer->cursor;
    { FILL_TOKEN(P_TOK_AMP_AMP); break; }
yy49:
    ++p_lexer->cursor;
    { FILL_TOKEN(P_TOK_AMP_EQUAL); break; }
yy50:
    ++p_lexer->cursor;
    { FILL_TOKEN(P_TOK_STAR_EQUAL); break; }
yy51:
    ++p_lexer->cursor;
    { FILL_TOKEN(P_TOK_PLUS_EQUAL); break; }
yy52:
    ++p_lexer->cursor;
    { FILL_TOKEN(P_TOK_MINUS_EQUAL); break; }
yy53:
    ++p_lexer->cursor;
    { FILL_TOKEN(P_TOK_ARROW); break; }
yy54:
    ++p_lexer->cursor;
    { goto block_comment; }
yy55:
    yych = *++p_lexer->cursor;
    switch (yych) {
        case 0x00:
        case '\n':
        case '\r': goto yy56;
        default: goto yy55;
    }
yy56:
    {
                if (!g_options.opt_verify_mode)
                    continue;

                FILL_TOKEN(P_TOK_COMMENT);
                p_token->data.literal.begin = p_lexer->marked_cursor;
                p_token->data.literal.end = p_lexer->cursor;
                break;
            }
yy57:
    ++p_lexer->cursor;
    { FILL_TOKEN(P_TOK_SLASH_EQUAL); break; }
yy58:
    yych = *++p_lexer->cursor;
    switch (yych) {
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9': goto yy75;
        default: goto yy59;
    }
yy59:
    p_lexer->cursor = p_lexer->marker;
    switch (yyaccept) {
        case 0: goto yy27;
        case 1: goto yy76;
        case 2: goto yy78;
        default: goto yy83;
    }
yy60:
    yych = *++p_lexer->cursor;
    switch (yych) {
        case '0':
        case '1': goto yy77;
        case '_': goto yy60;
        default: goto yy59;
    }
yy61:
    yych = *++p_lexer->cursor;
    switch (yych) {
        case '+':
        case '-': goto yy79;
        default: goto yy80;
    }
yy62:
    yych = *++p_lexer->cursor;
    switch (yych) {
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        case 'A':
        case 'B':
        case 'C':
        case 'D':
        case 'E':
        case 'F':
        case 'a':
        case 'b':
        case 'c':
        case 'd':
        case 'e':
        case 'f': goto yy82;
        case '_': goto yy62;
        default: goto yy59;
    }
yy63:
    yych = *++p_lexer->cursor;
    switch (yych) {
        case '3': goto yy84;
        case '6': goto yy85;
        default: goto yy59;
    }
yy64:
    yych = *++p_lexer->cursor;
    switch (yych) {
        case '1': goto yy86;
        case '3': goto yy87;
        case '6': goto yy88;
        case '8': goto yy89;
        default: goto yy59;
    }
yy65:
    yych = *++p_lexer->cursor;
    switch (yych) {
        case '=': goto yy90;
        default: goto yy66;
    }
yy66:
    { FILL_TOKEN(P_TOK_LESS_LESS); break; }
yy67:
    ++p_lexer->cursor;
    { FILL_TOKEN(P_TOK_LESS_EQUAL); break; }
yy68:
    ++p_lexer->cursor;
    { FILL_TOKEN(P_TOK_EQUAL_EQUAL); break; }
yy69:
    ++p_lexer->cursor;
    { FILL_TOKEN(P_TOK_GREATER_EQUAL); break; }
yy70:
    yych = *++p_lexer->cursor;
    switch (yych) {
        case '=': goto yy91;
        default: goto yy71;
    }
yy71:
    { FILL_TOKEN(P_TOK_GREATER_GREATER); break; }
yy72:
    ++p_lexer->cursor;
    { FILL_TOKEN(P_TOK_CARET_EQUAL); break; }
yy73:
    ++p_lexer->cursor;
    { FILL_TOKEN(P_TOK_PIPE_EQUAL); break; }
yy74:
    ++p_lexer->cursor;
    { FILL_TOKEN(P_TOK_PIPE_PIPE); break; }
yy75:
    yyaccept = 1;
    yych = *(p_lexer->marker = ++p_lexer->cursor);
    switch (yych) {
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        case '_': goto yy75;
        case 'E':
        case 'e': goto yy61;
        case 'f': goto yy63;
        default: goto yy76;
    }
yy76:
    {
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
yy77:
    yyaccept = 2;
    yych = *(p_lexer->marker = ++p_lexer->cursor);
    switch (yych) {
        case '0':
        case '1':
        case '_': goto yy77;
        case 'i':
        case 'u': goto yy92;
        default: goto yy78;
    }
yy78:
    {
                FILL_TOKEN(P_TOK_INT_LITERAL);
                p_token->data.literal.int_radix = 2;
                p_token->data.literal.begin = p_lexer->marked_cursor;
                p_token->data.literal.end = p_lexer->cursor;
                parse_int_suffix(p_token);
                break;
            }
yy79:
    yych = *++p_lexer->cursor;
yy80:
    switch (yych) {
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9': goto yy81;
        case '_': goto yy79;
        default: goto yy59;
    }
yy81:
    yyaccept = 1;
    yych = *(p_lexer->marker = ++p_lexer->cursor);
    switch (yych) {
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        case '_': goto yy81;
        case 'f': goto yy63;
        default: goto yy76;
    }
yy82:
    yyaccept = 3;
    yych = *(p_lexer->marker = ++p_lexer->cursor);
    switch (yych) {
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        case 'A':
        case 'B':
        case 'C':
        case 'D':
        case 'E':
        case 'F':
        case '_':
        case 'a':
        case 'b':
        case 'c':
        case 'd':
        case 'e':
        case 'f': goto yy82;
        case 'i':
        case 'u': goto yy93;
        default: goto yy83;
    }
yy83:
    {
                FILL_TOKEN(P_TOK_INT_LITERAL);
                p_token->data.literal.int_radix = 16;
                p_token->data.literal.begin = p_lexer->marked_cursor;
                p_token->data.literal.end = p_lexer->cursor;
                parse_int_suffix(p_token);
                break;
            }
yy84:
    yych = *++p_lexer->cursor;
    switch (yych) {
        case '2': goto yy94;
        default: goto yy59;
    }
yy85:
    yych = *++p_lexer->cursor;
    switch (yych) {
        case '4': goto yy94;
        default: goto yy59;
    }
yy86:
    yych = *++p_lexer->cursor;
    switch (yych) {
        case '6': goto yy89;
        default: goto yy59;
    }
yy87:
    yych = *++p_lexer->cursor;
    switch (yych) {
        case '2': goto yy89;
        default: goto yy59;
    }
yy88:
    yych = *++p_lexer->cursor;
    switch (yych) {
        case '4': goto yy89;
        default: goto yy59;
    }
yy89:
    ++p_lexer->cursor;
    goto yy27;
yy90:
    ++p_lexer->cursor;
    { FILL_TOKEN(P_TOK_LESS_LESS_EQUAL); break; }
yy91:
    ++p_lexer->cursor;
    { FILL_TOKEN(P_TOK_GREATER_GREATER_EQUAL); break; }
yy92:
    yych = *++p_lexer->cursor;
    switch (yych) {
        case '1': goto yy95;
        case '3': goto yy96;
        case '6': goto yy97;
        case '8': goto yy98;
        default: goto yy59;
    }
yy93:
    yych = *++p_lexer->cursor;
    switch (yych) {
        case '1': goto yy99;
        case '3': goto yy100;
        case '6': goto yy101;
        case '8': goto yy102;
        default: goto yy59;
    }
yy94:
    ++p_lexer->cursor;
    goto yy76;
yy95:
    yych = *++p_lexer->cursor;
    switch (yych) {
        case '6': goto yy98;
        default: goto yy59;
    }
yy96:
    yych = *++p_lexer->cursor;
    switch (yych) {
        case '2': goto yy98;
        default: goto yy59;
    }
yy97:
    yych = *++p_lexer->cursor;
    switch (yych) {
        case '4': goto yy98;
        default: goto yy59;
    }
yy98:
    ++p_lexer->cursor;
    goto yy78;
yy99:
    yych = *++p_lexer->cursor;
    switch (yych) {
        case '6': goto yy102;
        default: goto yy59;
    }
yy100:
    yych = *++p_lexer->cursor;
    switch (yych) {
        case '2': goto yy102;
        default: goto yy59;
    }
yy101:
    yych = *++p_lexer->cursor;
    switch (yych) {
        case '4': goto yy102;
        default: goto yy59;
    }
yy102:
    ++p_lexer->cursor;
    goto yy83;
}


        block_comment:
        
{
    char yych;
    yych = *p_lexer->cursor;
    switch (yych) {
        case 0x00: goto yy104;
        case '\n': goto yy107;
        case '\r': goto yy108;
        case '*': goto yy109;
        default: goto yy105;
    }
yy104:
    ++p_lexer->cursor;
    {
                // Unterminated block comment
                FILL_TOKEN(P_TOK_EOF);
                p_lexer->cursor--;
                break;
            }
yy105:
    ++p_lexer->cursor;
yy106:
    {
                goto block_comment;
            }
yy107:
    ++p_lexer->cursor;
    {
                register_new_line(p_lexer);
                goto block_comment;
            }
yy108:
    yych = *++p_lexer->cursor;
    switch (yych) {
        case '\n': goto yy107;
        default: goto yy106;
    }
yy109:
    yych = *++p_lexer->cursor;
    switch (yych) {
        case '/': goto yy110;
        default: goto yy106;
    }
yy110:
    ++p_lexer->cursor;
    {
                if (!g_options.opt_verify_mode)
                    continue;

                FILL_TOKEN(P_TOK_COMMENT);
                p_token->data.literal.begin = p_lexer->marked_cursor;
                p_token->data.literal.end = p_lexer->cursor;
                break;
            }
}

    }

#undef FILL_TOKEN
}
