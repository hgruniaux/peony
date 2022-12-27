#pragma once

#include <memory_resource>

/// This class implements a bump allocator.
///
/// From Rust <a href=https://docs.rs/bumpalo/latest/bumpalo/>bumpalo</a> package documentation:
/// <quote>
/// Bump allocation is a fast, but limited approach to allocation. We have a chunk of memory, and we maintain a pointer
/// within that memory. Whenever we allocate an object, we do a quick check that we have enough capacity left in our
/// chunk to allocate the object and then update the pointer by the object’s size. That’s it!
///
/// The disadvantage of bump allocation is that there is no general way to deallocate individual objects or reclaim the
/// memory region for a no-longer-in-use object.
///
/// These trade offs make bump allocation well-suited for phase-oriented allocations. That is, a group of objects that
/// will all be allocated during the same program phase, used, and then can all be deallocated together as a group.
/// </quote>
class PBumpAllocator
{
public:
  PBumpAllocator() = default;
  ~PBumpAllocator() noexcept = default;

  [[nodiscard]] void* alloc(size_t p_size, size_t p_alignment)
  {
    return m_buffer.allocate(p_size, p_alignment);
  }
  template<typename T>
  [[nodiscard]] T* alloc_with_extra_size(size_t p_extra_size)
  {
    return static_cast<T*>(alloc(sizeof(T) + p_extra_size, alignof(T)));
  }

  template<class T>
  [[nodiscard]] T* alloc_object(size_t p_n = 1)
  {
    return static_cast<T*>(alloc(sizeof(T) * p_n, alignof(T)));
  }
  template<class T, class... Args>
  [[nodiscard]] T* new_object(Args&&... p_args)
  {
    auto* ptr = alloc_object<T>();
    new (ptr) T(std::forward<Args>(p_args)...);
    return ptr;
  }

private:
  std::pmr::monotonic_buffer_resource m_buffer;
};
