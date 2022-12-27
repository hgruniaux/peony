#include "diag_formatter.hxx"
#include "../options.hxx"

#include <cassert>

static size_t
parse_integer(const char* p_begin, const char* p_end)
{
    size_t value = 0;
    while (p_begin != p_end) {
        value *= 10;
        value += *p_begin++ - '0';
    }
    return value;
}

void
p_diag_format_msg(std::string& p_buffer, const char* p_msg, PDiagArgument* p_args, size_t p_arg_count)
{
    const char* cursor = p_msg;
    const char* marker;

    for (;;) {
        const char* saved_cursor = cursor;
        /*!re2c
            re2c:yyfill:enable    = 0;
            re2c:encoding:utf8    = 0;
            re2c:indent:string    = "    ";
            re2c:define:YYCTYPE   = "char";

            re2c:define:YYCURSOR   = "cursor";
            re2c:define:YYMARKER   = "marker";

            integer = [0-9]+;

            "{" integer "}" {
                int arg_idx = parse_integer(saved_cursor + 1, cursor - 1);
                assert(arg_idx < p_arg_count);
                PDiagArgument arg = p_args[arg_idx];
                p_diag_format_arg(p_buffer, &arg);
                continue;
            }

            "%" integer "s" {
                int arg_idx = parse_integer(saved_cursor + 1, cursor - 1);
                assert(arg_idx < p_arg_count);
                assert(p_args[arg_idx].type == P_DAT_INT);
                if (p_args[arg_idx].value_int > 1) {
                    p_buffer.push_back('s');
                }

                continue;
            }

            "<%" {
                if (g_options.opt_diagnostics_color)
                    p_buffer.append("\x1b[1m");
                p_buffer.push_back('\'');
                continue;
            }

            "%>" {
                p_buffer.push_back('\'');
                if (g_options.opt_diagnostics_color)
                    p_buffer.append("\x1b[0m");
                continue;
            }
            
            "\x00" {
                break;
            }

            * {
                p_buffer.append(saved_cursor, cursor);
                continue;
            }
        */
    }
}
