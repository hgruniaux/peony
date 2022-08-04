#include "bump_allocator.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#if P_ENABLE_MEM_STATS
#include <stdio.h>
#endif

#define DEFAULT_SLAB_SIZE 4096

static bool
is_power_of_2(size_t p_value)
{
  return (p_value != 0) && ((p_value & (p_value - 1)) == 0);
}

struct PBumpAllocatorSlab*
create_slab(size_t p_size)
{
  assert(p_size > 0);

  struct PBumpAllocatorSlab* slab = malloc(sizeof(*slab) + p_size - 1);
  if (slab == NULL)
    return NULL;

  slab->prev = NULL;
  slab->end = slab->data + p_size;
  slab->ptr = slab->end;
  return slab;
}

/* Try to do the requested allocation on the given slab and if fails returns
 * NULL. */
static void*
try_alloc(struct PBumpAllocatorSlab* p_slab, size_t p_size, size_t p_align)
{
  uintptr_t new_ptr = (uintptr_t)(p_slab->ptr) - p_size;
  new_ptr &= -p_align;
  if (new_ptr < (uintptr_t)(p_slab->data)) {
    /* not enough space */
    return NULL;
  }

  p_slab->ptr = (char*)(new_ptr);
  return p_slab->ptr;
}

void
p_bump_init(struct PBumpAllocator* p_allocator)
{
  assert(p_allocator != NULL);

  p_allocator->last_slab = create_slab(DEFAULT_SLAB_SIZE);
  assert(p_allocator->last_slab != NULL);

#if P_ENABLE_MEM_STATS
  p_allocator->allocation_count = 0;
  p_allocator->bytes_allocated = 0;
  p_allocator->bytes_total = DEFAULT_SLAB_SIZE;
#endif
}

void
p_bump_destroy(struct PBumpAllocator* p_allocator)
{
  assert(p_allocator != NULL);

  struct PBumpAllocatorSlab* slab = p_allocator->last_slab;
  while (slab != NULL) {
    struct PBumpAllocatorSlab* tmp = slab;
    slab = slab->prev;
    free(tmp);
  }
}

void
p_bump_dump_stats(struct PBumpAllocator* p_allocator)
{
  assert(p_allocator != NULL);

#if P_ENABLE_MEM_STATS
  printf("Bump allocator stats:\n");
  printf("  allocation_count: %zu\n", p_allocator->allocation_count);
  printf("  bytes_allocated: %zu\n", p_allocator->bytes_allocated);
  printf("  bytes_total: %zu\n", p_allocator->bytes_total);
  printf("  waste: %f\n", 1.0 - ((double)p_allocator->bytes_allocated / (double)p_allocator->bytes_total));
#endif
}

void*
p_bump_alloc(struct PBumpAllocator* p_allocator, size_t p_size, size_t p_align)
{
  assert(p_allocator != NULL);
  assert(is_power_of_2(p_align));

#if P_ENABLE_MEM_STATS
  p_allocator->bytes_allocated += p_size;
  p_allocator->allocation_count++;
#endif

  struct PBumpAllocatorSlab* slab = p_allocator->last_slab;
  /* Try to allocate in an already existing slab. */
  while (slab != NULL) {
    void* ptr = try_alloc(slab, p_size, p_align);
    if (ptr != NULL)
      return ptr;
    slab = slab->prev;
  }

  size_t slab_size = DEFAULT_SLAB_SIZE;
  if (p_size > slab_size)
    slab_size = p_size;

  struct PBumpAllocatorSlab* new_slab = create_slab(slab_size);
  assert(new_slab != NULL);

#if P_ENABLE_MEM_STATS
  p_allocator->bytes_total += slab_size;
#endif

  new_slab->prev = p_allocator->last_slab;
  p_allocator->last_slab = new_slab;

  void* ptr = try_alloc(p_allocator->last_slab, p_size, p_align);
  assert(ptr != NULL);
  return ptr;
}
