#include "bump_allocator.hxx"

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

#if P_ENABLE_MEM_STATS
#include <cstdio>
#endif

struct PBumpAllocator p_global_bump_allocator;

[[nodiscard]] static inline bool
is_power_of_2(size_t p_value)
{
  return (p_value != 0) && ((p_value & (p_value - 1)) == 0);
}

struct PBumpAllocator::Slab
{
  static constexpr size_t kDEFAULT_SLAB_SIZE = 4096;

  Slab* prev;
  char* ptr;
  char* end;
  char data[1];

  [[nodiscard]] static Slab* create(size_t p_size = kDEFAULT_SLAB_SIZE) {
    assert(p_size > 0);

    auto* slab = static_cast<Slab*>(malloc(sizeof(Slab) + p_size - 1));
    if (slab == nullptr)
      return nullptr;

    slab->prev = nullptr;
    slab->end = slab->data + p_size;
    slab->ptr = slab->end;
    return slab;
  }

  // Try to do the requested allocation on the given slab and if fails returns nullptr.
  [[nodiscard]] void* try_alloc(size_t p_size, size_t p_align)
  {
    uintptr_t new_ptr = (uintptr_t)(ptr)-p_size;
    new_ptr &= -p_align;
    if (new_ptr < (uintptr_t)(data)) {
      // not enough space
      return nullptr;
    }

    ptr = (char*)(new_ptr);
    return ptr;
  }
};

PBumpAllocator::PBumpAllocator()
{
  m_last_slab = Slab::create();
  assert(m_last_slab != nullptr);

#if P_ENABLE_MEM_STATS
  m_allocation_count = 0;
  m_bytes_allocated = 0;
  m_bytes_total = Slab::kDEFAULT_SLAB_SIZE;
#endif
}

PBumpAllocator::~PBumpAllocator() noexcept
{
  Slab* slab = m_last_slab;
  while (slab != nullptr) {
    Slab* tmp = slab;
    slab = slab->prev;
    free(tmp);
  }
}

void*
PBumpAllocator::alloc(size_t p_size, size_t p_align)
{
  assert(is_power_of_2(p_align));

#if P_ENABLE_MEM_STATS
  m_bytes_allocated += p_size;
  m_allocation_count++;
#endif

  Slab* slab = m_last_slab;
  // Try to allocate in an already existing slab.
  while (slab != nullptr) {
    void* ptr = slab->try_alloc(p_size, p_align);
    if (ptr != nullptr)
      return ptr;
    slab = slab->prev;
  }

  size_t slab_size = Slab::kDEFAULT_SLAB_SIZE;
  if (p_size > slab_size)
    slab_size = p_size;

  auto* new_slab = Slab::create(slab_size);
  assert(new_slab != nullptr);

#if P_ENABLE_MEM_STATS
  m_bytes_total += slab_size;
#endif

  new_slab->prev = m_last_slab;
  m_last_slab = new_slab;

  void* ptr = m_last_slab->try_alloc(p_size, p_align);
  assert(ptr != nullptr);
  return ptr;
}

void
PBumpAllocator::dump_stats() const
{
#if P_ENABLE_MEM_STATS
  fprintf(stderr, "Bump allocator stats:\n");
  fprintf(stderr, "  allocation_count: %zu\n", m_allocation_count);
  fprintf(stderr, "  bytes_allocated: %zu\n", m_bytes_allocated);
  fprintf(stderr, "  bytes_total: %zu\n", m_bytes_total);
  fprintf(stderr, "  waste: %f\n", 1.0 - ((double)m_bytes_allocated / (double)m_bytes_total));
#endif
}
