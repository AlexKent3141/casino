#include "greatest/greatest.h"
#include "memory_tests.h"
#include "node_tests.h"

GREATEST_MAIN_DEFS();

int main(int argc, char** argv)
{
    GREATEST_MAIN_BEGIN();

    RUN_SUITE(MemoryTests);
    RUN_SUITE(NodeTests);

    GREATEST_MAIN_END();

    return 0;
}
