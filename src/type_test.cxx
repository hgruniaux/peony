#include "type.h"

#include <gtest/gtest.h>

TEST(type, paren_type)
{
  PType* f32_ty = p_type_get_f32();
  ASSERT_NE(f32_ty, nullptr);

  PParenType* paren_ty = (PParenType*)p_type_get_paren(f32_ty);
  ASSERT_NE(paren_ty, nullptr);

  EXPECT_EQ(paren_ty->sub_type, f32_ty);

  EXPECT_FALSE(p_type_is_canonical((PType*)paren_ty));
  EXPECT_EQ(p_type_get_canonical((PType*)paren_ty), f32_ty);

  PParenType* paren_two_ty = (PParenType*)p_type_get_paren((PType*)paren_ty);
  ASSERT_NE(paren_two_ty, nullptr);

  EXPECT_EQ(paren_two_ty->sub_type, (PType*)paren_ty);

  EXPECT_FALSE(p_type_is_canonical((PType*)paren_two_ty));
  EXPECT_EQ(p_type_get_canonical((PType*)paren_two_ty), f32_ty);
}

TEST(type, function_type)
{
  PType* ret_ty = p_type_get_paren(p_type_get_f32());
  PType* args_ty[] = { p_type_get_i32(), p_type_get_paren(p_type_get_bool()) };

  PFunctionType* func_ty = (PFunctionType*)p_type_get_function(ret_ty, args_ty, 2);
  ASSERT_NE(func_ty, nullptr);

  EXPECT_EQ(func_ty->ret_type, ret_ty);
  ASSERT_EQ(func_ty->arg_count, 2);
  EXPECT_EQ(func_ty->args[0], args_ty[0]);
  EXPECT_EQ(func_ty->args[1], args_ty[1]);
  EXPECT_FALSE(p_type_is_canonical((PType*)func_ty));

  PType* canonical_ret_ty = p_type_get_canonical(ret_ty);
  PType* canonical_args_ty[] = { p_type_get_canonical(args_ty[0]), p_type_get_canonical(args_ty[1]) };
  PFunctionType* canonical_func_ty = (PFunctionType*)p_type_get_function(canonical_ret_ty, canonical_args_ty, 2);
  ASSERT_NE(canonical_func_ty, nullptr);

  EXPECT_EQ(canonical_func_ty->ret_type, canonical_ret_ty);
  ASSERT_EQ(canonical_func_ty->arg_count, 2);
  EXPECT_EQ(canonical_func_ty->args[0], canonical_args_ty[0]);
  EXPECT_EQ(canonical_func_ty->args[1], canonical_args_ty[1]);
  EXPECT_TRUE(p_type_is_canonical((PType*)canonical_func_ty));

  EXPECT_EQ(p_type_get_canonical((PType*)canonical_func_ty), (PType*)canonical_func_ty);
  EXPECT_EQ(p_type_get_canonical((PType*)func_ty), (PType*)canonical_func_ty);

  PFunctionType* func_bis_ty = (PFunctionType*)p_type_get_function(ret_ty, args_ty, 2);
  ASSERT_NE(func_bis_ty, nullptr);
  // There must be only one instance of each pointer type.
  EXPECT_EQ(func_ty, func_bis_ty);
}

TEST(type, pointer_type)
{
  PType* f32_ty = p_type_get_f32();
  ASSERT_NE(f32_ty, nullptr);
  PPointerType* pointer_ty = (PPointerType*)p_type_get_pointer(f32_ty);
  ASSERT_NE(pointer_ty, nullptr);

  EXPECT_EQ(pointer_ty->element_type, f32_ty);
  // Pointer types are already canonical.
  EXPECT_TRUE(p_type_is_canonical((PType*)pointer_ty));
  EXPECT_EQ(p_type_get_canonical((PType*)pointer_ty), (PType*)pointer_ty);

  PPointerType* pointer_bis_ty = (PPointerType*)p_type_get_pointer(f32_ty);
  ASSERT_NE(pointer_bis_ty, nullptr);
  // There must be only one instance of each pointer type.
  EXPECT_EQ(pointer_ty, pointer_bis_ty);
}
