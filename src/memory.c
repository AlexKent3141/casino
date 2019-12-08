#include "memory.h"
#include "string.h"
#include "assert.h"
#include "stdio.h"

struct MemoryState* MemoryInit(size_t bufSize, char* buf)
{
    struct MemoryState* mem;

    if (bufSize < sizeof(struct MemoryState))
        return NULL;

    mem = (struct MemoryState*)&buf[0];
    mem->buf = buf;
    mem->bufSize = bufSize;
    mem->bufNext = sizeof(struct MemoryState);

    return mem;
}

bool IsMemoryAvailable(struct MemoryState* st, size_t numBytes)
{
    return numBytes <= st->bufSize - st->bufNext;
}

char* GetMemory(struct MemoryState* st, size_t numBytes)
{
    char* buf;

    if (!IsMemoryAvailable(st, numBytes))
        return NULL;

    buf = &st->buf[st->bufNext];
    assert(buf != NULL);
    memset(buf, 0, numBytes);
    st->bufNext += numBytes;
    return buf;
}
