#include "context.hxx"
#include "type.hxx"

#include <gtest/gtest.h>

TEST(Type, paren_type)
{
  auto& ctx = PContext::get_global();

  PType* f32_ty = ctx.get_f32_ty();
  ASSERT_NE(f32_ty, nullptr);

  PParenType* paren_ty = ctx.get_paren_ty(f32_ty);
  ASSERT_NE(paren_ty, nullptr);

  EXPECT_EQ(paren_ty->get_sub_type(), f32_ty);

  EXPECT_FALSE(paren_ty->is_canonical_ty());
  EXPECT_EQ(paren_ty->get_canonical_ty(), f32_ty);

  PParenType* paren_two_ty = ctx.get_paren_ty((PType*)paren_ty);
  ASSERT_NE(paren_two_ty, nullptr);

  EXPECT_EQ(paren_two_ty->get_sub_type(), (PType*)paren_ty);

  EXPECT_FALSE(paren_two_ty->is_canonical_ty());
  EXPECT_EQ(paren_two_ty->get_canonical_ty(), f32_ty);
}

TEST(Type, function_type)
{
  auto& ctx = PContext::get_global();

  PType* ret_ty = ctx.get_paren_ty(ctx.get_f32_ty());
  PType* args_ty[] = { ctx.get_i32_ty(), ctx.get_paren_ty(ctx.get_bool_ty()) };

  PFunctionType* func_ty = ctx.get_function_ty(ret_ty, { args_ty, 2 });
  ASSERT_NE(func_ty, nullptr);

  EXPECT_EQ(func_ty->get_ret_ty(), ret_ty);
  ASSERT_EQ(func_ty->get_param_count(), 2);
  EXPECT_EQ(func_ty->get_params()[0], args_ty[0]);
  EXPECT_EQ(func_ty->get_params()[1], args_ty[1]);
  EXPECT_FALSE(func_ty->is_canonical_ty());

  PType* canonical_ret_ty = ret_ty->get_canonical_ty();
  PType* canonical_args_ty[] = { args_ty[0]->get_canonical_ty(), args_ty[1]->get_canonical_ty() };
  PFunctionType* canonical_func_ty = ctx.get_function_ty(canonical_ret_ty, { canonical_args_ty, 2 });
  ASSERT_NE(canonical_func_ty, nullptr);

  EXPECT_EQ(canonical_func_ty->get_ret_ty(), canonical_ret_ty);
  ASSERT_EQ(canonical_func_ty->get_param_count(), 2);
  EXPECT_EQ(canonical_func_ty->get_params()[0], canonical_args_ty[0]);
  EXPECT_EQ(canonical_func_ty->get_params()[1], canonical_args_ty[1]);
  EXPECT_TRUE(canonical_func_ty->is_canonical_ty());

  EXPECT_EQ(canonical_func_ty->get_canonical_ty(), canonical_func_ty);
  EXPECT_EQ(func_ty->get_canonical_ty(), canonical_func_ty);

  PFunctionType* func_bis_ty = ctx.get_function_ty(ret_ty, { args_ty, 2 });
  ASSERT_NE(func_bis_ty, nullptr);
  // There must be only one instance of each pointer type.
  EXPECT_EQ(func_ty, func_bis_ty);
}

TEST(Type, pointer_type)
{
  auto& ctx = PContext::get_global();

  PType* f32_ty = ctx.get_f32_ty();
  ASSERT_NE(f32_ty, nullptr);
  PPointerType* pointer_ty = ctx.get_pointer_ty(f32_ty);
  ASSERT_NE(pointer_ty, nullptr);

  EXPECT_EQ(pointer_ty->get_element_ty(), f32_ty);
  EXPECT_TRUE(pointer_ty->is_canonical_ty());
  EXPECT_EQ(pointer_ty->get_canonical_ty(), pointer_ty);

  // There must be only one instance of each pointer type.
  PPointerType* pointer_bis_ty = ctx.get_pointer_ty(f32_ty);
  EXPECT_EQ(pointer_ty, pointer_bis_ty);

  // Non-canonical pointer type
  PType* paren_ty = ctx.get_paren_ty(f32_ty);
  ASSERT_NE(paren_ty, nullptr);
  PPointerType* non_canonical_pointer_ty = ctx.get_pointer_ty(paren_ty);
  ASSERT_NE(non_canonical_pointer_ty, nullptr);
  EXPECT_EQ(non_canonical_pointer_ty->get_element_ty(), paren_ty);
  EXPECT_NE(non_canonical_pointer_ty, pointer_ty);
  EXPECT_FALSE(non_canonical_pointer_ty->is_canonical_ty());
  EXPECT_EQ(non_canonical_pointer_ty->get_canonical_ty(), pointer_ty);
}

TEST(Type, array_type)
{
  auto& ctx = PContext::get_global();

  PType* f32_ty = ctx.get_f32_ty();
  ASSERT_NE(f32_ty, nullptr);
  PArrayType* array_ty = ctx.get_array_ty(f32_ty, 3);
  ASSERT_NE(array_ty, nullptr);

  EXPECT_EQ(array_ty->get_element_ty(), f32_ty);
  EXPECT_EQ(array_ty->get_num_elements(), 3);
  EXPECT_TRUE(array_ty->is_canonical_ty());
  EXPECT_EQ(array_ty->get_canonical_ty(), array_ty);

  // There must be only one instance of each array type.
  PArrayType* pointer_bis_ty = ctx.get_array_ty(f32_ty, 3);
  PArrayType* pointer_bis2_ty = ctx.get_array_ty(f32_ty, 4);
  EXPECT_EQ(array_ty, pointer_bis_ty);
  EXPECT_NE(array_ty, pointer_bis2_ty); // however, array of different sizes are different types

  // Non-canonical array type
  PType* paren_ty = ctx.get_paren_ty(f32_ty);
  ASSERT_NE(paren_ty, nullptr);
  PArrayType* non_canonical_array_ty = ctx.get_array_ty(paren_ty, 3);
  ASSERT_NE(non_canonical_array_ty, nullptr);
  EXPECT_EQ(non_canonical_array_ty->get_element_ty(), paren_ty);
  EXPECT_EQ(non_canonical_array_ty->get_num_elements(), 3);
  EXPECT_NE(non_canonical_array_ty, array_ty);
  EXPECT_FALSE(non_canonical_array_ty->is_canonical_ty());
  EXPECT_EQ(non_canonical_array_ty->get_canonical_ty(), array_ty);
}

TEST(Type, is_predicates)
{
  auto& ctx = PContext::get_global();

  // i: is_int_ty()
  // s: is_signed_int_ty()
  // u: is_unsigned_int_ty()
  // f: is_float_ty()
  // a: is_arithmetic_ty()

  std::pair<PType*, const char*> types[] = {
    { ctx.get_i8_ty(), "isa" }, { ctx.get_i16_ty(), "isa" }, { ctx.get_i32_ty(), "isa" }, { ctx.get_i64_ty(), "isa" },
    { ctx.get_u8_ty(), "iua" }, { ctx.get_u16_ty(), "iua" }, { ctx.get_u32_ty(), "iua" }, { ctx.get_u64_ty(), "iua" },
    { ctx.get_f32_ty(), "fa" }, { ctx.get_f64_ty(), "fa" },
  };

  for (auto item : types) {
    while (*item.second != '\0') {
      switch (*item.second++) {
        case 'i':
          EXPECT_TRUE(item.first->is_int_ty());
          EXPECT_TRUE(item.first->is_signed_int_ty() || item.first->is_unsigned_int_ty());
          break;
        case 's':
          EXPECT_TRUE(item.first->is_signed_int_ty());
          EXPECT_FALSE(item.first->is_unsigned_int_ty());
          break;
        case 'u':
          EXPECT_TRUE(item.first->is_unsigned_int_ty());
          EXPECT_FALSE(item.first->is_signed_int_ty());
          break;
        case 'f':
          EXPECT_TRUE(item.first->is_float_ty());
          EXPECT_FALSE(item.first->is_int_ty());
          EXPECT_FALSE(item.first->is_unsigned_int_ty());
          EXPECT_FALSE(item.first->is_signed_int_ty());
          break;
        case 'a':
          EXPECT_TRUE(item.first->is_arithmetic_ty());
          break;
      }
    }
  }
}

TEST(Type, is_predicates_non_canonical_type)
{
  // Type::is_*() predicates must test canonical id.
  // That is `((int))`.is_int_ty() must returns true (note the parenthesizes).

  auto& ctx = PContext::get_global();

  auto* ty0 = ctx.get_paren_ty(ctx.get_i32_ty());
  EXPECT_TRUE(ty0->is_int_ty());
  EXPECT_TRUE(ty0->is_signed_int_ty());
  EXPECT_FALSE(ty0->is_unsigned_int_ty());
  EXPECT_TRUE(ty0->is_arithmetic_ty());

  auto* ty1 = ctx.get_paren_ty(ctx.get_u32_ty());
  EXPECT_TRUE(ty1->is_int_ty());
  EXPECT_FALSE(ty1->is_signed_int_ty());
  EXPECT_TRUE(ty1->is_unsigned_int_ty());
  EXPECT_TRUE(ty1->is_arithmetic_ty());

  auto* ty2 = ctx.get_paren_ty(ctx.get_f32_ty());
  EXPECT_TRUE(ty2->is_float_ty());
  EXPECT_FALSE(ty2->is_int_ty());
  EXPECT_TRUE(ty2->is_arithmetic_ty());

  auto* ty3 = ctx.get_paren_ty(ctx.get_pointer_ty(ctx.get_i32_ty()));
  EXPECT_TRUE(ty3->is_pointer_ty());

  auto* ty4 = ctx.get_paren_ty(ctx.get_function_ty(ty2, {}));
  EXPECT_TRUE(ty4->is_function_ty());
}

TEST(Type, to_signed_unsigned_int_ty)
{
  auto& ctx = PContext::get_global();

  // to_signed_int_ty()
  EXPECT_EQ(ctx.get_i8_ty()->to_signed_int_ty(ctx), ctx.get_i8_ty());
  EXPECT_EQ(ctx.get_i16_ty()->to_signed_int_ty(ctx), ctx.get_i16_ty());
  EXPECT_EQ(ctx.get_i32_ty()->to_signed_int_ty(ctx), ctx.get_i32_ty());
  EXPECT_EQ(ctx.get_i64_ty()->to_signed_int_ty(ctx), ctx.get_i64_ty());
  EXPECT_EQ(ctx.get_u8_ty()->to_signed_int_ty(ctx), ctx.get_i8_ty());
  EXPECT_EQ(ctx.get_u16_ty()->to_signed_int_ty(ctx), ctx.get_i16_ty());
  EXPECT_EQ(ctx.get_u32_ty()->to_signed_int_ty(ctx), ctx.get_i32_ty());
  EXPECT_EQ(ctx.get_u64_ty()->to_signed_int_ty(ctx), ctx.get_i64_ty());

  // to_unsigned_int_ty()
  EXPECT_EQ(ctx.get_i8_ty()->to_unsigned_int_ty(ctx), ctx.get_u8_ty());
  EXPECT_EQ(ctx.get_i16_ty()->to_unsigned_int_ty(ctx), ctx.get_u16_ty());
  EXPECT_EQ(ctx.get_i32_ty()->to_unsigned_int_ty(ctx), ctx.get_u32_ty());
  EXPECT_EQ(ctx.get_i64_ty()->to_unsigned_int_ty(ctx), ctx.get_u64_ty());
  EXPECT_EQ(ctx.get_u8_ty()->to_unsigned_int_ty(ctx), ctx.get_u8_ty());
  EXPECT_EQ(ctx.get_u16_ty()->to_unsigned_int_ty(ctx), ctx.get_u16_ty());
  EXPECT_EQ(ctx.get_u32_ty()->to_unsigned_int_ty(ctx), ctx.get_u32_ty());
  EXPECT_EQ(ctx.get_u64_ty()->to_unsigned_int_ty(ctx), ctx.get_u64_ty());
}
