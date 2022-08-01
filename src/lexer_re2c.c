/* Generated by re2c */
#include "lexer.h"

#include "utils/diag.h"

#include <assert.h>
#include <stdio.h>



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

        
{
    char yych;
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
        case '#': goto yy10;
        case '%': goto yy12;
        case '&': goto yy14;
        case '(': goto yy16;
        case ')': goto yy17;
        case '*': goto yy18;
        case '+': goto yy20;
        case ',': goto yy22;
        case '-': goto yy23;
        case '/': goto yy25;
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9': goto yy27;
        case ':': goto yy29;
        case ';': goto yy30;
        case '<': goto yy31;
        case '=': goto yy33;
        case '>': goto yy35;
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
        case 'z': goto yy37;
        case '^': goto yy39;
        case '{': goto yy41;
        case '|': goto yy42;
        case '}': goto yy44;
        default: goto yy2;
    }
yy1:
    ++p_lexer->cursor;
    {
                FILL_TOKEN(P_TOK_EOF);
                break;
            }
yy2:
    ++p_lexer->cursor;
    {
                error("unknown character '%c'\n", p_lexer->cursor[-1]);
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
                ++CURRENT_LINENO;

                uint32_t line_pos = p_lexer->cursor - p_lexer->source_file->buffer;
                p_line_map_add(&p_lexer->source_file->line_map, line_pos);
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
        case '=': goto yy45;
        default: goto yy9;
    }
yy9:
    { FILL_TOKEN(P_TOK_EXCLAIM); break; }
yy10:
    yych = *++p_lexer->cursor;
    switch (yych) {
        case 0x00:
        case '\n':
        case '\r': goto yy11;
        default: goto yy10;
    }
yy11:
    {
                if (!g_verify_mode_enabled)
                    continue;

                FILL_TOKEN(P_TOK_COMMENT);
                p_token->data.literal_begin = p_lexer->marked_cursor + 1;
                p_token->data.literal_end = p_lexer->cursor;
                break;
            }
yy12:
    yych = *++p_lexer->cursor;
    switch (yych) {
        case '=': goto yy46;
        default: goto yy13;
    }
yy13:
    { FILL_TOKEN(P_TOK_PERCENT); break; }
yy14:
    yych = *++p_lexer->cursor;
    switch (yych) {
        case '&': goto yy47;
        case '=': goto yy48;
        default: goto yy15;
    }
yy15:
    { FILL_TOKEN(P_TOK_AMP); break; }
yy16:
    ++p_lexer->cursor;
    { FILL_TOKEN(P_TOK_LPAREN); break; }
yy17:
    ++p_lexer->cursor;
    { FILL_TOKEN(P_TOK_RPAREN); break; }
yy18:
    yych = *++p_lexer->cursor;
    switch (yych) {
        case '=': goto yy49;
        default: goto yy19;
    }
yy19:
    { FILL_TOKEN(P_TOK_STAR); break; }
yy20:
    yych = *++p_lexer->cursor;
    switch (yych) {
        case '=': goto yy50;
        default: goto yy21;
    }
yy21:
    { FILL_TOKEN(P_TOK_PLUS); break; }
yy22:
    ++p_lexer->cursor;
    { FILL_TOKEN(P_TOK_COMMA); break; }
yy23:
    yych = *++p_lexer->cursor;
    switch (yych) {
        case '=': goto yy51;
        case '>': goto yy52;
        default: goto yy24;
    }
yy24:
    { FILL_TOKEN(P_TOK_MINUS); break; }
yy25:
    yych = *++p_lexer->cursor;
    switch (yych) {
        case '=': goto yy53;
        default: goto yy26;
    }
yy26:
    { FILL_TOKEN(P_TOK_SLASH); break; }
yy27:
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
        case '9': goto yy27;
        default: goto yy28;
    }
yy28:
    {
                FILL_TOKEN(P_TOK_INT_LITERAL);
                p_token->data.literal_begin = p_lexer->marked_cursor;
                p_token->data.literal_end = p_lexer->cursor;
                break;
            }
yy29:
    ++p_lexer->cursor;
    { FILL_TOKEN(P_TOK_COLON); break; }
yy30:
    ++p_lexer->cursor;
    { FILL_TOKEN(P_TOK_SEMI); break; }
yy31:
    yych = *++p_lexer->cursor;
    switch (yych) {
        case '<': goto yy54;
        case '=': goto yy56;
        default: goto yy32;
    }
yy32:
    { FILL_TOKEN(P_TOK_LESS); break; }
yy33:
    yych = *++p_lexer->cursor;
    switch (yych) {
        case '=': goto yy57;
        default: goto yy34;
    }
yy34:
    { FILL_TOKEN(P_TOK_EQUAL); break; }
yy35:
    yych = *++p_lexer->cursor;
    switch (yych) {
        case '=': goto yy58;
        case '>': goto yy59;
        default: goto yy36;
    }
yy36:
    { FILL_TOKEN(P_TOK_GREATER); break; }
yy37:
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
        case 'z': goto yy37;
        default: goto yy38;
    }
yy38:
    {
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
yy39:
    yych = *++p_lexer->cursor;
    switch (yych) {
        case '=': goto yy61;
        default: goto yy40;
    }
yy40:
    { FILL_TOKEN(P_TOK_CARET); break; }
yy41:
    ++p_lexer->cursor;
    { FILL_TOKEN(P_TOK_LBRACE); break; }
yy42:
    yych = *++p_lexer->cursor;
    switch (yych) {
        case '=': goto yy62;
        case '|': goto yy63;
        default: goto yy43;
    }
yy43:
    { FILL_TOKEN(P_TOK_PIPE); break; }
yy44:
    ++p_lexer->cursor;
    { FILL_TOKEN(P_TOK_RBRACE); break; }
yy45:
    ++p_lexer->cursor;
    { FILL_TOKEN(P_TOK_EXCLAIM_EQUAL); break; }
yy46:
    ++p_lexer->cursor;
    { FILL_TOKEN(P_TOK_PERCENT_EQUAL); break; }
yy47:
    ++p_lexer->cursor;
    { FILL_TOKEN(P_TOK_AMP_AMP); break; }
yy48:
    ++p_lexer->cursor;
    { FILL_TOKEN(P_TOK_AMP_EQUAL); break; }
yy49:
    ++p_lexer->cursor;
    { FILL_TOKEN(P_TOK_STAR_EQUAL); break; }
yy50:
    ++p_lexer->cursor;
    { FILL_TOKEN(P_TOK_PLUS_EQUAL); break; }
yy51:
    ++p_lexer->cursor;
    { FILL_TOKEN(P_TOK_MINUS_EQUAL); break; }
yy52:
    ++p_lexer->cursor;
    { FILL_TOKEN(P_TOK_ARROW); break; }
yy53:
    ++p_lexer->cursor;
    { FILL_TOKEN(P_TOK_SLASH_EQUAL); break; }
yy54:
    yych = *++p_lexer->cursor;
    switch (yych) {
        case '=': goto yy64;
        default: goto yy55;
    }
yy55:
    { FILL_TOKEN(P_TOK_LESS_LESS); break; }
yy56:
    ++p_lexer->cursor;
    { FILL_TOKEN(P_TOK_LESS_EQUAL); break; }
yy57:
    ++p_lexer->cursor;
    { FILL_TOKEN(P_TOK_EQUAL_EQUAL); break; }
yy58:
    ++p_lexer->cursor;
    { FILL_TOKEN(P_TOK_GREATER_EQUAL); break; }
yy59:
    yych = *++p_lexer->cursor;
    switch (yych) {
        case '=': goto yy65;
        default: goto yy60;
    }
yy60:
    { FILL_TOKEN(P_TOK_GREATER_GREATER); break; }
yy61:
    ++p_lexer->cursor;
    { FILL_TOKEN(P_TOK_CARET_EQUAL); break; }
yy62:
    ++p_lexer->cursor;
    { FILL_TOKEN(P_TOK_PIPE_EQUAL); break; }
yy63:
    ++p_lexer->cursor;
    { FILL_TOKEN(P_TOK_PIPE_PIPE); break; }
yy64:
    ++p_lexer->cursor;
    { FILL_TOKEN(P_TOK_LESS_LESS_EQUAL); break; }
yy65:
    ++p_lexer->cursor;
    { FILL_TOKEN(P_TOK_GREATER_GREATER_EQUAL); break; }
}

    }
}
