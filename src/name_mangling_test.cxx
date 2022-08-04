#include "name_mangling.h"

#include <gtest/gtest.h>

class name_mangling : public ::testing::Test
{
protected:
  void SetUp() override { p_identifier_table_init(&identifier_table); }
  void TearDown() override { p_identifier_table_destroy(&identifier_table); }

  void check_mangled_name(const char* name, PType* func_type, const char* expected_mangled_name)
  {
    PIdentifierInfo* ident = p_identifier_table_get(&identifier_table, name, nullptr);
    PString mangled_name;
    string_init(&mangled_name);
    name_mangle(ident, (PFunctionType*)func_type, &mangled_name);
    EXPECT_STREQ(mangled_name.buffer, expected_mangled_name);
    string_destroy(&mangled_name);
  }

  PIdentifierTable identifier_table;
};

TEST_F(name_mangling, builtin_types)
{
  PType* args[] = { p_type_get_char(), p_type_get_bool(), p_type_get_i8(),  p_type_get_u8(),
                    p_type_get_i16(),  p_type_get_u16(),  p_type_get_i32(), p_type_get_u32(),
                    p_type_get_i64(),  p_type_get_u64(),  p_type_get_f32(), p_type_get_f64() };
  const size_t args_size = sizeof(args) / sizeof(*args);
  PType* func_type = p_type_get_function(p_type_get_void(), args, args_size);

  check_mangled_name("foo", func_type, "_P3foovcbahstlmxyfd");
}

TEST_F(name_mangling, pointer_type)
{
  PType* args[] = { p_type_get_pointer(p_type_get_bool()) };
  const size_t args_size = sizeof(args) / sizeof(*args);
  PType* func_type = p_type_get_function(p_type_get_pointer(p_type_get_pointer(p_type_get_f32())), args, args_size);

  check_mangled_name("ptr_func", func_type, "_P8ptr_funcPPfPb");
}

TEST_F(name_mangling, func_type)
{
  PType* args1[] = { p_type_get_f32(), p_type_get_paren(p_type_get_bool()) };
  const size_t args1_size = sizeof(args1) / sizeof(*args1);
  PType* func1_type = p_type_get_function(p_type_get_paren(p_type_get_i32()), args1, args1_size);

  PType* args2[] = { func1_type };
  const size_t args2_size = sizeof(args2) / sizeof(*args2);
  PType* func2_type = p_type_get_function(p_type_get_void(), args2, args2_size);
  check_mangled_name("func_with_func_arg", func2_type, "_P18func_with_func_argvFlfbE");
}
