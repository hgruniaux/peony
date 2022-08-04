#include "name_mangling.h"

#include "ast.h"

#include <assert.h>

static void
append_int(PString* p_output, size_t p_value)
{
  if (p_value == 0) {
    string_append(p_output, "0");
    return;
  }

  char buffer[21]; // should be enough to encode UINT64_MAX + NUL-terminator
  char* ptr = buffer;
  do {
    *ptr++ = "0123456789"[(p_value % 10)];
    p_value /= 10;
  } while (p_value > 0);

  // reverse string
  *ptr-- = '\0';
  char* ptr1 = buffer;
  while (buffer < ptr) {
    char ch = *ptr;
    *ptr-- = *ptr1;
    *ptr1++ = ch;
  }

  string_append(p_output, buffer);
}

static void
append_ident(PString* p_output, PIdentifierInfo* p_ident)
{
  append_int(p_output, p_ident->spelling_len);
  string_append(p_output, p_ident->spelling);
}

static void
append_type(PString* p_output, PType* p_type)
{
  assert(p_type_is_canonical(p_type) && !p_type_is_generic(p_type));

  const char* builtin_types[] = {
    "v", // 'void'
    "c", // 'char'
    "b", // 'bool'
    "a", // 'i8'
    "s", // 'i16'
    "l", // 'i32'
    "x", // 'i64'
    "h", // 'u8'
    "t", // 'u16'
    "m", // 'u32'
    "y", // 'u64'
    "f", // 'f32'
    "d"  // 'f64'
  };

  switch (P_TYPE_GET_KIND(p_type)) {
    case P_TYPE_POINTER:
      string_append(p_output, "P");
      append_type(p_output, ((PPointerType*)p_type)->element_type);
      break;
    case P_TYPE_FUNCTION:
      string_append(p_output, "F");
      append_type(p_output, ((PFunctionType*)p_type)->ret_type);
      for (int i = 0; i < ((PFunctionType*)p_type)->arg_count; ++i) {
        append_type(p_output, ((PFunctionType*)p_type)->args[i]);
      }
      string_append(p_output, "E");
      break;
    case P_TYPE_TAG:
      append_ident(p_output, P_DECL_GET_NAME(((PTagType*)p_type)->decl));
      break;
    default:
      // Is a builtin type
      string_append(p_output, builtin_types[P_TYPE_GET_KIND(p_type)]);
      break;
  }
}

// Basic structure of mangled name:
//   - Prefix "_P"
//   - Name
//   - Type information
//
// Name mangling specification:
//   <mangled-name> ::= _P <encoding>
//
//   <encoding> ::= <name> <func-sig> # <name> is the function name
//
//   <name> ::= <number> <identifier> # <number> is the length of <identifier> and <identifier>
//                                      is a source code identifier
//
//   <func-sig> ::= <type>+ # return type then the parameter types in order left-to-right
//
// Type encoding:
//   <type> ::= <builtin-type>
//          ::= P <type> # pointer
//          ::= F <func-sig> E # function
//
// Builtin types:
//   <builtin-type> ::= v # void
//                  ::= c # char
//                  ::= b # bool
//                  ::= a # i8
//                  ::= h # u8
//                  ::= s # i16
//                  ::= t # u16
//                  ::= l # i32
//                  ::= m # u32
//                  ::= x # i64
//                  ::= y # u64
//                  ::= i # isize
//                  ::= j # usize
//                  ::= n # i128
//                  ::= o # u128
//                  ::= f # f32
//                  ::= d # f64
//
// Examples:
//   * fn foo(x: i32) -> bool ==> _P3foobl
void
name_mangle(PIdentifierInfo* p_name, PFunctionType* p_func_type, PDynamicArray* p_output)
{
  p_func_type = (PFunctionType*)p_type_get_canonical((PType*)p_func_type);

  string_append(p_output, "_P");
  append_ident(p_output, p_name);

  append_type(p_output, p_func_type->ret_type);
  for (int i = 0; i < p_func_type->arg_count; ++i) {
    append_type(p_output, p_func_type->args[i]);
  }
}
