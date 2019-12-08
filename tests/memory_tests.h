#ifndef __CASINO_MEMORY_TESTS_INCLUDED_H__
#define __CASINO_MEMORY_TESTS_INCLUDED_H__

#include "greatest/greatest.h"
#include "../src/memory.h"
#include "stdlib.h"

/*
 * Check that a memory state can be initialised if enough memory is allocated.
 */
TEST MemoryInitSuccess_Test()
{
    struct MemoryState* mem;
    size_t bufSize;
    char* buf;

    bufSize = sizeof(struct MemoryState);
    buf = malloc(bufSize);
    mem = MemoryInit(bufSize, buf);

    ASSERT(mem != NULL);

    free(buf);

    PASS();
}

/*
 * Check that memory state initialisation fails if insufficient memory is allocated.
 */
TEST MemoryInitFail_Test()
{
    struct MemoryState* mem;
    size_t bufSize;
    char* buf;

    bufSize = sizeof(struct MemoryState) - 1;
    buf = malloc(bufSize);
    mem = MemoryInit(bufSize, buf);

    ASSERT(mem == NULL);

    free(buf);

    PASS();
}

/*
 * Check that memory is report as available when it is.
 */
TEST IsMemoryAvailable_Test()
{
    struct MemoryState* mem;
    size_t bufSize;
    char* buf;

    /* Allocate enough memory for a MemoryState with an extra 4 bytes. */
    bufSize = sizeof(struct MemoryState) + 4;
    buf = malloc(bufSize);
    mem = MemoryInit(bufSize, buf);

    ASSERT(IsMemoryAvailable(mem, 4));
    ASSERT(!IsMemoryAvailable(mem, 8));

    free(buf);

    PASS();
}

TEST GetMemory_Test()
{
    struct MemoryState* mem;
    size_t bufSize;
    char* buf, *testBuf;

    /* Allocate enough memory for a MemoryState with an extra 4 bytes. */
    bufSize = sizeof(struct MemoryState) + 4;
    buf = malloc(bufSize);
    mem = MemoryInit(bufSize, buf);

    /* Should be able to get another 4 bytes - any more should fail. */
    testBuf = GetMemory(mem, 4);
    ASSERT(testBuf != NULL);

    testBuf = GetMemory(mem, 4);
    ASSERT(testBuf == NULL);

    free(buf);

    PASS();
}

SUITE(MemoryTests)
{
    RUN_TEST(MemoryInitSuccess_Test);
    RUN_TEST(MemoryInitFail_Test);
    RUN_TEST(IsMemoryAvailable_Test);
    RUN_TEST(GetMemory_Test);
}

#endif /* __CASINO_MEMORY_TESTS_INCLUDED_H__ */
