#include "hash_table_common.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

size_t
p_get_new_hash_table_size(size_t p_old_size)
{
  /* From https://www.planetmath.org/goodhashtableprimes */
  static size_t prime_sizes[] = {
    53,        97,        193,       389,       769,        1543,     3079,
    6151,      12289,     24593,     49157,     98317,      196613,   393241,
    786433,    1572869,   3145739,   6291469,   12582917,   25165843, 50331653,
    100663319, 201326611, 402653189, 805306457, 1610612741,
  };

  const size_t prime_count = sizeof(prime_sizes) / sizeof(prime_sizes[0]);

  for (size_t i = 0; i < prime_count; ++i) {
    if (prime_sizes[i] > p_old_size)
      return prime_sizes[i];
  }

  /* If there is not enough prime sizes just fallback to a naive growth
   * strategy. */
  return p_old_size * 2;
}

void**
p_alloc_buckets(size_t p_count, size_t p_bucket_size)
{
  const size_t size = p_count * p_bucket_size;
  void** buckets = malloc(size);
  assert(buckets != NULL);
  memset(buckets, 0, size);
  return buckets;
}
