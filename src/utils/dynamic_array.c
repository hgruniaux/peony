#include "dynamic_array.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_CAPACITY 16
#define GROWTH_FACTOR 2
#define ALLOC_BUFFER(p_size, p_item_size) (malloc((p_size) * (p_item_size)))

void
dyn_array_init_impl(PDynamicArray* p_array, size_t p_item_size)
{
  assert(p_array != NULL);

  p_array->size = 0;
  p_array->capacity = INITIAL_CAPACITY;
  p_array->buffer = ALLOC_BUFFER(p_array->capacity, p_item_size);
  assert(p_array->buffer != NULL);
}

void
dyn_array_destroy_impl(PDynamicArray* p_array)
{
  assert(p_array != NULL);

  free(p_array->buffer);
  p_array->buffer = NULL;
  p_array->capacity = 0;
  p_array->size = 0;
}

void
dyn_array_append_impl(PDynamicArray* p_array, size_t p_item_size, void* p_item)
{
  if (p_array->size >= p_array->capacity) {
    p_array->capacity *= GROWTH_FACTOR;
    PDynamicArrayItem* new_buffer = ALLOC_BUFFER(p_array->capacity, p_item_size);
    assert(new_buffer != NULL);
    memcpy(new_buffer, p_array->buffer, p_item_size * p_array->size);
    free(p_array->buffer);
    p_array->buffer = new_buffer;
  }

  memcpy(p_array->buffer + p_array->size++ * p_item_size, p_item, p_item_size);
}
