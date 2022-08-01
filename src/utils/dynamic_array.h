#pragma once

#include <stddef.h>

#include "hedley.h"

HEDLEY_BEGIN_C_DECLS

/*
 * This file implements a generic dynamic array that stores
 * pointers.
 */

typedef void* PDynamicArrayItem;

typedef struct PDynamicArray
{
  PDynamicArrayItem* buffer;
  size_t size;
  size_t capacity;
} PDynamicArray;

void
p_dynamic_array_init(PDynamicArray* p_array);
void
p_dynamic_array_destroy(PDynamicArray* p_array);
void
p_dynamic_array_append(PDynamicArray* p_array, PDynamicArrayItem p_item);

HEDLEY_END_C_DECLS
