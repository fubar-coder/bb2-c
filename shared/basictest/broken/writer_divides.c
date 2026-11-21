/*
 * writer_divides.c - deliberately broken input of the writer check
 * (check-writer.cmake, ctest basictest_writer_check_broken): the writer
 * with a decimal routine that divides, which on the 68000 calls libgcc's
 * __udivsi3 and __umodsi3.
 */
#include "../basictest_write.c"

size_t basictest_decimal_divides(uint32_t v, uint8_t *buf);

size_t basictest_decimal_divides(uint32_t v, uint8_t *buf)
{
    uint8_t rev[10];
    size_t n = 0, i;

    do {
        rev[n++] = (uint8_t)('0' + v % 10);
        v /= 10;
    } while (v);
    for (i = 0; i < n; i++)
        buf[i] = rev[n - 1 - i];
    return n;
}
