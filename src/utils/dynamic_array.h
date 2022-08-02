#pragma once

#include <hedley.h>

#include <stddef.h>

HEDLEY_BEGIN_C_DECLS

/*
 * This file implements a generic dynamic array that stores pointers.
 */

typedef void* PDynamicArrayItem;

typedef struct PDynamicArray
{
  char* buffer;
  size_t size;
  size_t capacity;
} PDynamicArray;

void
dyn_array_init_impl(PDynamicArray* p_array, size_t p_item_size);
void
dyn_array_destroy_impl(PDynamicArray* p_array);
void
dyn_array_append_impl(PDynamicArray* p_array, size_t p_item_size, void* p_item);

#define DYN_ARRAY_INIT(p_type, p_array) (dyn_array_init_impl((p_array), sizeof(p_type)))
#define DYN_ARRAY_DESTROY(p_type, p_array) (dyn_array_destroy_impl((p_array)))
#define DYN_ARRAY_APPEND(p_type, p_array, p_item) (dyn_array_append_impl((p_array), sizeof(p_type), (p_type*)&(p_item)))
#define DYN_ARRAY_AT(p_type, p_array, p_idx) (*((p_type*)((p_array)->buffer + sizeof(p_type) * (p_idx))))

HEDLEY_END_C_DECLS
