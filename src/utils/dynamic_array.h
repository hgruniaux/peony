#pragma once

#include <hedley.h>

#include <stddef.h>

HEDLEY_BEGIN_C_DECLS

// PDynamicArray is a generic dynamic array implementation.
//
// Use the DYN_ARRAY_*() macros to use. They all take a parameter the
// type of elements in the dynamic array and do all the dirty work for
// you. The elements stored in the dynamic array must be trivially
// copyable.
//
// To access the individual elements of the array use DYN_ARRAY_AT()
// instead of directly with the 'buffer' field. However, you can
// directly access the 'size' field without risks.
typedef struct PDynamicArray
{
  // The internal buffer. Do not use it directly, access elements using
  // DYN_ARRAY_AT() macro.
  char* buffer;
  // Number of elements actually stored in the array (and not the size in
  // bytes of the buffer).
  size_t size;
  // Number of elements that can be actually stored in the array.
  size_t capacity;
} PDynamicArray;

void
dyn_array_init_impl(PDynamicArray* p_array, size_t p_item_size);
void
dyn_array_destroy_impl(PDynamicArray* p_array);
void
dyn_array_reserve_impl(PDynamicArray* p_array, size_t p_item_size, size_t p_capacity);
void
dyn_array_append_impl(PDynamicArray* p_array, size_t p_item_size, void* p_item);

#define DYN_ARRAY_INIT(p_type, p_array) (dyn_array_init_impl((p_array), sizeof(p_type)))
#define DYN_ARRAY_DESTROY(p_type, p_array) (dyn_array_destroy_impl((p_array)))
#define DYN_ARRAY_APPEND(p_type, p_array, p_item) (dyn_array_append_impl((p_array), sizeof(p_type), (p_type*)&(p_item)))
#define DYN_ARRAY_AT(p_type, p_array, p_idx) (*((p_type*)((p_array)->buffer + sizeof(p_type) * (p_idx))))

HEDLEY_END_C_DECLS
