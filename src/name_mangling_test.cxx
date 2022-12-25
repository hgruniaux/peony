#include "name_mangling.hxx"
#include "context.hxx"

#include <gtest/gtest.h>

class name_mangling : public ::testing::Test
{
protected:
  void SetUp() override {
    p_identifier_table_init(&identifier_table);
  }
  void TearDown() override {
    p_identifier_table_destroy(&identifier_table);
  }

  void check_mangled_name(const char* name, PType* func_type, const char* expected_mangled_name)
  {
    PIdentifierInfo* ident = p_identifier_table_get(&identifier_table, name, nullptr);
    std::string mangled_name;
    name_mangle(ident, (PFunctionType*)func_type, mangled_name);
    EXPECT_STREQ(mangled_name.data(), expected_mangled_name);
  }

  PIdentifierTable identifier_table;
};

TEST_F(name_mangling, builtin_types)
{
  auto& ctx = PContext::get_global();
  PType* args[] = { ctx.get_char_ty(), ctx.get_bool_ty(), ctx.get_i8_ty(),  ctx.get_u8_ty(),
                    ctx.get_i16_ty(),  ctx.get_u16_ty(),  ctx.get_i32_ty(), ctx.get_u32_ty(),
                    ctx.get_i64_ty(),  ctx.get_u64_ty(),  ctx.get_f32_ty(), ctx.get_f64_ty() };
  PType* func_type = ctx.get_function_ty(ctx.get_void_ty(), args, std::size(args));

  check_mangled_name("foo", func_type, "_P3foovcbahstlmxyfd");
}

TEST_F(name_mangling, pointer_type)
{
  auto& ctx = PContext::get_global();
  PType* args[] = { ctx.get_pointer_ty(ctx.get_bool_ty()) };
  PType* func_type = ctx.get_function_ty(ctx.get_pointer_ty(ctx.get_pointer_ty(ctx.get_f32_ty())), args, std::size(args));

  check_mangled_name("ptr_func", func_type, "_P8ptr_funcPPfPb");
}

TEST_F(name_mangling, func_type)
{
  auto& ctx = PContext::get_global();
  PType* args1[] = { ctx.get_f32_ty(), ctx.get_paren_ty(ctx.get_bool_ty()) };
  PType* func1_type = ctx.get_function_ty(ctx.get_paren_ty(ctx.get_i32_ty()), args1, std::size(args1));

  PType* args2[] = { func1_type };
  PType* func2_type = ctx.get_function_ty(ctx.get_void_ty(), args2, std::size(args2));
  check_mangled_name("func_with_func_arg", func2_type, "_P18func_with_func_argvFlfbE");
}
