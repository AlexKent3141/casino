#include "memory.h"
#include "assert.h"
#include "string.h"

struct MemoryState* MemoryInit(
    size_t bufSize,
    char* buf)
{
    struct MemoryState* mem;

    if (bufSize < sizeof(struct MemoryState))
        return NULL;

    mem = (struct MemoryState*)&buf[0];

    rmk_mutex_create(&mem->mutex);

    mem->buf = buf;
    mem->bufSize = bufSize;
    mem->bufNext = sizeof(struct MemoryState);
    mem->bufRoot = 0;

    return mem;
}

void ResetTree(
    struct MemoryState* st)
{
    if (st->bufRoot != 0)
    {
        st->bufNext = st->bufRoot;
    }
}

bool IsMemoryAvailable(
    struct MemoryState* st,
    size_t numBytes)
{
    return numBytes <= st->bufSize - st->bufNext;
}

char* GetMemory(
    struct MemoryState* st,
    size_t numBytes)
{
    char* buf;

    rmk_mutex_lock(&st->mutex);

    if (!IsMemoryAvailable(st, numBytes))
    {
        rmk_mutex_unlock(&st->mutex);
        return NULL;
    }

    buf = &st->buf[st->bufNext];
    assert(buf != NULL);
    memset(buf, 0, numBytes);
    st->bufNext += numBytes;

    rmk_mutex_unlock(&st->mutex);

    return buf;
}
