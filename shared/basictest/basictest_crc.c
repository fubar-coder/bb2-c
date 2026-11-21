/*
 * basictest_crc.c - the CRC32 of a string value of a `checkstr` entry
 * (basictest.h): the standard CRC-32 of IEEE 802.3, as in zlib and PNG -
 * the polynomial $04C11DB7 reflected ($EDB88320), input and output
 * reflected, the register $FFFFFFFF at the start and the result XORed
 * with $FFFFFFFF.  Compiled by the test library with the Blitz library
 * conventions and by the host tools: no libc, no libgcc, no writable
 * static data - four bits a step through a table of 16 longs (64 bytes),
 * shifts and XOR only.
 */
#include "basictest.h"

/* the CRC of the 16 values of four bits: entry i is i shifted out four
   times, XORed with $EDB88320 whenever a 1 bit leaves */
static const uint32_t nibble[16] = {
    0x00000000UL, 0x1DB71064UL, 0x3B6E20C8UL, 0x26D930ACUL,
    0x76DC4190UL, 0x6B6B51F4UL, 0x4DB26158UL, 0x5005713CUL,
    0xEDB88320UL, 0xF00F9344UL, 0xD6D6A3E8UL, 0xCB61B38CUL,
    0x9B64C2B0UL, 0x86D3D2D4UL, 0xA00AE278UL, 0xBDBDF21CUL
};

uint32_t basictest_crc32(const uint8_t *data, size_t length)
{
    uint32_t crc = 0xFFFFFFFFUL;
    size_t i;

    for (i = 0; i < length; i++) {
        crc ^= data[i];
        crc = (crc >> 4) ^ nibble[crc & 15];
        crc = (crc >> 4) ^ nibble[crc & 15];
    }
    return crc ^ 0xFFFFFFFFUL;
}
