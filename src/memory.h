#ifndef __CASINO_MEMORY_INCLUDED_H__
#define __CASINO_MEMORY_INCLUDED_H__

#include "../include/casino.h"
#include "pthread.h"
#include "stdlib.h"
#include "stdbool.h"

struct MemoryState
{
    char* buf;      /* The block of memory. */
    size_t bufSize; /* The size of the buffer. */
    size_t bufNext; /* The next available (unused) position in the buffer. */
    size_t bufRoot; /* The position of the latest root node. */
    pthread_mutex_t mutex;
};

struct MemoryState* MemoryInit(size_t bufSize, char* buf);

/*
 * This method is used to reset the tree between searches.
 */
void ResetTree(struct MemoryState* st);

bool IsMemoryAvailable(struct MemoryState* st, size_t numBytes);

/*
 * Get a block of memory of the required size.
 * If there isn't enough memory available return NULL.
 */
char* GetMemory(struct MemoryState* st, size_t numBytes);

#endif /* __CASINO_MEMORY_INCLUDED_H__ */
