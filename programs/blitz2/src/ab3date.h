/*
 * ab3date.h - the date of AmiBlitz3's escapes \\__DATE__ and
 * \\__DATE_GER__ (blitz2-ab3, ab3str.c; spec ab3-compat/strings, design
 * D6 of the change ab3-mode-g3): dos.library's day number (days since
 * 1978-01-01, DateStamp's ds_Days) as a calendar date and as AmiBlitz3's
 * text, without the locale.
 *
 * Pure C on <stdint.h>, no AmigaOS call: blitz2-ab3 compiles it, and the
 * host test (programs/blitz2/tests/host/test_ab3date.cpp) compiles it
 * with the native compiler.
 */
#ifndef AB3DATE_H
#define AB3DATE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief The bytes of a date text with its 0 (ab3_datetext); enough for
 *         any day number. */
#define AB3DATE_TEXTLEN 16

/**
 * @brief The calendar date (Gregorian) of a day number.
 * @param days   days since 1978-01-01 (0: that day)
 * @param year   the year
 * @param month  the month, 1..12
 * @param day    the day of the month, 1..31
 */
void ab3_daydate(uint32_t days, uint32_t *year, uint8_t *month, uint8_t *day);

/**
 * @brief The date of a day number as AmiBlitz3's escapes give it: month,
 *        day and year (\\__DATE__, "mm.dd.yyyy") or day, month and year
 *        (\\__DATE_GER__, "dd.mm.yyyy"); month and day with two digits,
 *        the year in decimal.
 * @param days  days since 1978-01-01
 * @param ger   0: mm.dd.yyyy, else dd.mm.yyyy
 * @param buf   AB3DATE_TEXTLEN bytes: the text with its 0
 * @return the length of the text
 */
unsigned ab3_datetext(uint32_t days, int ger, char *buf);

#ifdef __cplusplus
}
#endif

#endif
