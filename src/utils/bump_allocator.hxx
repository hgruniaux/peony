#pragma once

#include <hedley.h>

#include <cstddef>

#define P_ENABLE_MEM_STATS 1

/// This class implements a bump allocator.
///
/// From Rust <a href=https://docs.rs/bumpalo/latest/bumpalo/>bumpalo</a> package documentation:
/// <quote>
/// Bump allocation is a fast, but limited approach to allocation. We have a chunk of memory, and we maintain a pointer within that memory. Whenever we allocate an object, we do a quick check that we have enough capacity left in our chunk to allocate the object and then update the pointer by the object’s size. That’s it!
///
/// The disadvantage of bump allocation is that there is no general way to deallocate individual objects or reclaim the memory region for a no-longer-in-use object.
///
/// These trade offs make bump allocation well-suited for phase-oriented allocations. That is, a group of objects that will all be allocated during the same program phase, used, and then can all be deallocated together as a group.
/// </quote>
class PBumpAllocator
{
public:
  PBumpAllocator();
  ~PBumpAllocator() noexcept;

  [[nodiscard]] void* alloc(size_t p_size, size_t p_align);
  template <typename T>
  [[nodiscard]] T* alloc() { return static_cast<T*>(alloc(sizeof(T), alignof(T))); }
  template <typename T>
  [[nodiscard]] T* alloc_with_extra_size(size_t p_extra_size) { return static_cast<T*>(alloc(sizeof(T) + p_extra_size, alignof(T))); }

  /// Dumps memory statistics to stderr. If the macro P_ENABLE_MEM_STATS is undefined
  /// or defined to false, nothing will be printed.
  void dump_stats() const;

private:
  struct Slab;
  Slab* m_last_slab;

  /* Stats: */
#if P_ENABLE_MEM_STATS
size_t m_allocation_count;
size_t m_bytes_allocated;
size_t m_bytes_total;
#endif
};

extern struct PBumpAllocator p_global_bump_allocator;
