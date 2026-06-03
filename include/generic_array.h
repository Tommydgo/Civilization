#ifndef GENERIC_ARRAY_H
#define GENERIC_ARRAY_H

#include <stdlib.h>
#include "constants.h"

/*
 * DEFINE_ARRAY(T, Name)
 *
 * Generates a typed dynamic array for any complete type T.
 * All six declarations/definitions are file-scope; invoke only at top level.
 *
 * Usage:
 *   DEFINE_ARRAY(Unit, Unit)    →  UnitArray, UnitArray_init, ...
 *   DEFINE_ARRAY(int, Int)      →  IntArray,  IntArray_init,  ...
 */
#define DEFINE_ARRAY(T, Name) \
typedef struct { \
    T *data; \
    int count; \
    int capacity; \
} Name##Array; \
\
static inline void Name##Array_init(Name##Array *a) \
{ \
    a->data = malloc(sizeof(T) * ARRAY_INITIAL_CAPACITY); \
    a->count = 0; \
    a->capacity = ARRAY_INITIAL_CAPACITY; \
} \
\
static inline void Name##Array_push(Name##Array *a, T item) \
{ \
    if (a->count >= a->capacity) { \
        a->capacity *= 2; \
        a->data = realloc(a->data, sizeof(T) * a->capacity); \
    } \
    a->data[a->count++] = item; \
} \
\
static inline void Name##Array_remove(Name##Array *a, int i) \
{ \
    a->data[i] = a->data[--a->count]; \
} \
\
static inline void Name##Array_free(Name##Array *a) \
{ \
    free(a->data); \
    a->data = NULL; \
    a->count = 0; \
    a->capacity = 0; \
}

#endif
