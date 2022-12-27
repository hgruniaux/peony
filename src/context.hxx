#ifndef PEONY_CONTEXT_HXX
#define PEONY_CONTEXT_HXX

#include "type.hxx"
#include "utils/bump_allocator.hxx"

#include <span>
#include <unordered_map>
#include <vector>

class PContext
{
public:
  static PContext& get_global();

  [[nodiscard]] PBumpAllocator& get_allocator() { return m_allocator; }
  [[nodiscard]] const PBumpAllocator& get_allocator() const { return m_allocator; }

  // Allocation functions:
  [[nodiscard]] void* alloc(size_t p_size, size_t p_align) { return m_allocator.alloc(p_size, p_align); }
  template<class T>
  [[nodiscard]] T* alloc_object(size_t p_n = 1)
  {
    return m_allocator.alloc_object<T>(p_n);
  }
  template<class T, class... Args>
  [[nodiscard]] T* new_object(Args&&... p_args)
  {
    return m_allocator.new_object<T>(std::forward<Args>(p_args)...);
  }

  [[nodiscard]] PType* get_void_ty() { return &m_void_ty; }
  [[nodiscard]] PType* get_char_ty() { return &m_char_ty; }
  [[nodiscard]] PType* get_bool_ty() { return &m_bool_ty; }
  [[nodiscard]] PType* get_i8_ty() { return &m_i8_ty; }
  [[nodiscard]] PType* get_i16_ty() { return &m_i16_ty; }
  [[nodiscard]] PType* get_i32_ty() { return &m_i32_ty; }
  [[nodiscard]] PType* get_i64_ty() { return &m_i64_ty; }
  [[nodiscard]] PType* get_u8_ty() { return &m_u8_ty; }
  [[nodiscard]] PType* get_u16_ty() { return &m_u16_ty; }
  [[nodiscard]] PType* get_u32_ty() { return &m_u32_ty; }
  [[nodiscard]] PType* get_u64_ty() { return &m_u64_ty; }
  [[nodiscard]] PType* get_f32_ty() { return &m_f32_ty; }
  [[nodiscard]] PType* get_f64_ty() { return &m_f64_ty; }

  [[nodiscard]] PParenType* get_paren_ty(PType* p_sub_type);
  [[nodiscard]] PFunctionType* get_function_ty(PType* p_ret_ty, PType** p_params, size_t p_param_count);
  [[nodiscard]] PFunctionType* get_function_ty(PType* p_ret_ty, const std::vector<PType*>& p_args);
  [[nodiscard]] PPointerType* get_pointer_ty(PType* p_elt_ty);
  [[nodiscard]] PArrayType* get_array_ty(PType* p_elt_ty, size_t p_num_elements);

private:
  PBumpAllocator m_allocator;

  // Builtin types:
  PType m_void_ty{ PTypeKind::P_TK_VOID };
  PType m_char_ty{ PTypeKind::P_TK_CHAR };
  PType m_bool_ty{ PTypeKind::P_TK_BOOL };
  PType m_i8_ty{ PTypeKind::P_TK_I8 };
  PType m_i16_ty{ PTypeKind::P_TK_I16 };
  PType m_i32_ty{ PTypeKind::P_TK_I32 };
  PType m_i64_ty{ PTypeKind::P_TK_I64 };
  PType m_u8_ty{ PTypeKind::P_TK_U8 };
  PType m_u16_ty{ PTypeKind::P_TK_U16 };
  PType m_u32_ty{ PTypeKind::P_TK_U32 };
  PType m_u64_ty{ PTypeKind::P_TK_U64 };
  PType m_f32_ty{ PTypeKind::P_TK_F32 };
  PType m_f64_ty{ PTypeKind::P_TK_F64 };

  // Composite types:

  [[nodiscard]] static inline size_t hash_combine(size_t p_lhs, size_t p_rhs) noexcept
  {
    return p_lhs ^ (p_rhs + 0x9e3779b9 + (p_lhs << 6) + (p_lhs >> 2));
  }

  struct FuncTyKey
  {
    PType* ret_ty;
    PType** arg_types;
    size_t arg_count;

    [[nodiscard]] bool operator==(const FuncTyKey& p_key) const noexcept
    {
      if (ret_ty != p_key.ret_ty || arg_count != p_key.arg_count)
        return false;

      for (size_t i = 0; i < arg_count; ++i) {
        if (arg_types[i] != p_key.arg_types[i])
          return false;
      }

      return true;
    }
  };

  /// Implementation of the std::hash interface for FuncTyKey
  struct FuncTyKeyHash
  {
    [[nodiscard]] size_t operator()(const FuncTyKey& p_key) const noexcept
    {
      auto hasher = std::hash<PType*>{};
      size_t hval = hasher(p_key.ret_ty);
      for (size_t i = 0; i < p_key.arg_count; ++i) {
        hval = hash_combine(hval, hasher(p_key.arg_types[i]));
      }
      return hval;
    }
  };

  std::unordered_map<FuncTyKey, PFunctionType*, FuncTyKeyHash> m_func_tys;
  std::unordered_map<PType*, PPointerType*> m_pointer_tys;

  /// Implementation of the std::hash interface for std::pair<PType*, size_t>.
  struct PairHash
  {
    [[nodiscard]] size_t operator()(std::pair<PType*, size_t> p_val) const noexcept
    {
      const size_t lhs = std::hash<PType*>{}(p_val.first);
      const size_t rhs = std::hash<size_t>{}(p_val.second);
      return hash_combine(lhs, rhs);
    }
  };

  std::unordered_map<std::pair<PType*, size_t>, PArrayType*, PairHash> m_array_tys;
};

#endif // PEONY_CONTEXT_HXX
