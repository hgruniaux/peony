#pragma once

#include <stddef.h>

/*
 * This file contains common functions and constants to all hash table
 * implementations that can be found in this source tree.
 *
 * FIXME: Merge all different hash table implementations.
 */

#define P_MAX_LOAD_FACTOR 0.7f
#define P_GET_LOAD_FACTOR(p_table)                                             \
  ((float)(p_table)->item_count / (float)(p_table)->bucket_count)
#define P_NEEDS_REHASHING(p_table)                                             \
  (P_GET_LOAD_FACTOR(p_table) > P_MAX_LOAD_FACTOR)

/* Returns a new suitable count of buckets for a growing hash
 * table. */
size_t
p_get_new_hash_table_size(size_t p_old_size);

void**
p_alloc_buckets(size_t p_count, size_t p_bucket_size);

#define P_ALLOC_BUCKETS(p_count, p_type)                                       \
  (p_type*)p_alloc_buckets((p_count), sizeof(p_type))
