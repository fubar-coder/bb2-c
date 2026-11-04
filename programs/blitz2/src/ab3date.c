/*
 * ab3date.c - dos.library's day number as a date (ab3date.h): the value of
 * the escapes \\__DATE__ and \\__DATE_GER__ (ab3str.c, design D6 of the
 * change ab3-mode-g3).  AmiBlitz3 takes SystemDate's Date$ and replaces
 * its '/' by '.' (AmiBlitz3.ab3:26934); no locale here, the same text.
 */
#include "ab3date.h"

/* days from 0000-03-01 (proleptic Gregorian) to 1978-01-01 */
#define DAYS_TO_1978 722390UL
/* the days of 400 years */
#define DAYS_400Y 146097UL

void ab3_daydate(uint32_t days, uint32_t *year, uint8_t *month, uint8_t *day)
{
    /* the calendar counted from 0000-03-01, so a leap day is the last day
       of its year: 400-year eras, the year of the era, then the day of the
       year from March (153 days per 5 months) */
    uint32_t z = days + DAYS_TO_1978;
    uint32_t era = z / DAYS_400Y;
    uint32_t doe = z - era * DAYS_400Y;
    uint32_t yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    uint32_t doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    uint32_t mp = (5 * doy + 2) / 153;
    uint32_t m = mp < 10 ? mp + 3 : mp - 9;

    *year = era * 400 + yoe + (m <= 2);
    *month = (uint8_t)m;
    *day = (uint8_t)(doy - (153 * mp + 2) / 5 + 1);
}

/* two digits of v (0..99) at p */
static char *two(char *p, unsigned v)
{
    *p++ = (char)('0' + v / 10);
    *p++ = (char)('0' + v % 10);
    return p;
}

unsigned ab3_datetext(uint32_t days, int ger, char *buf)
{
    char digits[10], *p = buf;
    uint32_t y;
    uint8_t m, d;
    unsigned n = 0;

    ab3_daydate(days, &y, &m, &d);
    p = two(p, ger ? d : m);
    *p++ = '.';
    p = two(p, ger ? m : d);
    *p++ = '.';
    do
        digits[n++] = (char)('0' + y % 10);
    while (y /= 10);
    while (n)
        *p++ = digits[--n];
    *p = 0;
    return (unsigned)(p - buf);
}
