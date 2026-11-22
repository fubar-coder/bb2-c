/*
 * hangstub - a program that never ends: the fake compiler of the prev
 * configuration in compiler_selftest_lost_*_prev (tests/corpus/CMakeLists.txt),
 * whose session ends in a timeout without a result, independent of any
 * bug of the compiler.
 */
#include <exec/types.h>
#include <proto/exec.h>

int main(void)
{
    Wait(0);
    return 0;
}
