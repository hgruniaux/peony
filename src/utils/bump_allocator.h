#pragma once

#include <hedley.h>

#include <stddef.h>

#define P_ENABLE_MEM_STATS 1

HEDLEY_BEGIN_C_DECLS

struct PBumpAllocatorSlab
{
  struct PBumpAllocatorSlab* prev;
  char* ptr;
  char* end;
  char data[1];
};

struct PBumpAllocator
{
  struct PBumpAllocatorSlab* last_slab;

  /* Stats: */
#if P_ENABLE_MEM_STATS
  size_t allocation_count;
  size_t bytes_allocated;
  size_t bytes_total;
#endif
};

/* Initializes the allocator. Must be called before any other functions using
 * p_allocator. */
void
p_bump_init(struct PBumpAllocator* p_allocator);

/* Frees all memory allocated until now and destroys the allocator.
 * Using p_allocator after this function is undefined behavior. */
void
p_bump_destroy(struct PBumpAllocator* p_allocator);

/* Dumps some stats about the allocator if compiled with P_ENABLE_MEM_STATS=1,
 * otherwise does nothing. */
void
p_bump_dump_stats(struct PBumpAllocator* p_allocator);

/* Allocates a chunk of memory with the requested size (in bytes) and
 * alignment (in bytes). */
void*
p_bump_alloc(struct PBumpAllocator* p_allocator, size_t p_size, size_t p_align);

#if defined(__cplusplus) && __cplusplus >= 201103L
#define P_ALIGNOF(p_type) alignof(p_type)
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#define P_ALIGNOF(p_type) _Alignof(p_type)
#elif defined(__GNUC__)
#define P_ALIGNOF(p_type) __alignof__(p_type)
#elif defined(_MSC_VER)
#define P_ALIGNOF(p_type) __alignof(p_type)
#else
#define P_ALIGNOF(p_type) ((size_t)1)
#endif

#define P_BUMP_ALLOC(p_allocator, p_type) p_bump_alloc((p_allocator), sizeof(p_type), P_ALIGNOF(p_type))

// FIXME: move this into a better place
#define P_ARRAY_LENGTH(array) (sizeof(array) / sizeof((array)[0]))

extern struct PBumpAllocator p_global_bump_allocator;

HEDLEY_END_C_DECLS
