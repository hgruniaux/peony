#include "type.hxx"
#include "context.hxx"

#include <gtest/gtest.h>

TEST(type, paren_type)
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

TEST(type, function_type)
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

TEST(type, pointer_type)
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

TEST(type, array_type)
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
