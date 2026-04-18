#ifndef clox_memory_h
#define clox_memory_h

#include "common.h"

// calculates a new capacity based on a given current capacity
#define GROW_CAPACITY(capacity) \
    ((capacity) < 8 ? 8 : (capacity) * 2)


#define GROW_ARRAY(type, pointer, oldCount, newCount) \
    (type*)reallocate(pointer, sizeof(type) *(oldCount), \
    sizeof(type) * (newCount))

// single function used in Clox for all dynamic memory management
// routing all of those operation through a single function will be
// important later when we add a garbage collector
void *reallocate(void* pointer, size_t oldSize, size_t newSize);

// frees memory by passing in zero for the new size
#define FREE_ARRAY(type, pointer, oldCount)\
    reallocate(pointer, sizeof(type) * (oldCount), 0)
#endif