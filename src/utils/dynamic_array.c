#include "dynamic_array.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_CAPACITY 16
#define GROWTH_FACTOR 2
#define ALLOC_BUFFER(p_size) (malloc(sizeof(PDynamicArrayItem) * (p_size)))

void
p_dynamic_array_init(PDynamicArray* p_array)
{
  assert(p_array != NULL);

  p_array->size = 0;
  p_array->capacity = INITIAL_CAPACITY;
  p_array->buffer = ALLOC_BUFFER(p_array->capacity);
  assert(p_array->buffer != NULL);
}

void
p_dynamic_array_destroy(PDynamicArray* p_array)
{
  assert(p_array != NULL);

  free(p_array->buffer);
  p_array->buffer = NULL;
  p_array->capacity = 0;
  p_array->size = 0;
}

void
p_dynamic_array_append(PDynamicArray* p_array, PDynamicArrayItem p_item)
{
  assert(p_array != NULL);

  if (p_array->size >= p_array->capacity) {
    p_array->capacity *= GROWTH_FACTOR;
    PDynamicArrayItem* new_buffer = ALLOC_BUFFER(p_array->capacity);
    assert(new_buffer != NULL);
    memcpy(
      new_buffer, p_array->buffer, sizeof(PDynamicArrayItem) * p_array->size);
    free(p_array->buffer);
    p_array->buffer = new_buffer;
  }

  p_array->buffer[p_array->size++] = p_item;
}
