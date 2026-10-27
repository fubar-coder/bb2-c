/*
 * datelib - command library 78: dates as days since 1/1/1978 and the
 * system time (SystemDate, Date$, NumDays, DateFormat, Days, Months,
 * Years, WeekDay, Hours, Mins, Secs).
 *
 * C translation of bblibsrc/datelib (the CD binary matches the source).
 * Dates are counted from day 0 = 1/1/0000 in 4 year cycles of 1461 days
 * (every 4th year a leap year, also 1900/2100 ...), the month table is
 * the cumulative one of a leap year.  The string format is dd/mm/yyyy or
 * (DateFormat 1) mm/dd/yyyy.
 *
 * The routines do their arithmetic with word operations and DIVU/DIVS;
 * to stay bit-exact also for invalid input, the C code below follows the
 * register operations of the original: values are kept as 32 bit
 * "registers", word/byte operations change only the low word/byte
 * (SETW/SETB), DIVU/DIVS are the instructions (overflow leaves the
 * register unchanged).
 *
 * Kept on purpose (BUG(orig)):
 *   - Date$: the day after the leap day period of a cycle is wrong: day
 *     366 of a cycle (1/1 of the year after a leap year, e.g. 1/1/1981)
 *     is shown as 32/12 of the leap year (CMP #366: BLE instead of BLT).
 *   - WeekDay: SUBQ.w #2 on the day count (no borrow into the high word).
 *   - NumDays: ADDQ.w #1 / SUBQ.w #1 on the day count (no carry/borrow),
 *     e.g. NumDays("31/12/1977") = 65535, not -1.
 *   - NumDays: a 2 digit year reads past the end of the string (the year
 *     ends only at "/" or after 4 characters); a "/" as first character
 *     of a field becomes the value 47; month 0 reads the low word of the
 *     tick count of the last DateStamp (word before the month table),
 *     months 14/15 read the two words after the table (MOVEQ #0,d0:RTS at
 *     the end of the original library, reproduced below).
 *   - Mins/Secs return the values of the last Hours call (no DateStamp
 *     of their own), as documented.
 */
#include "blitzlib.h"
#include <dos/dos.h>
#define __NOLIBBASE__
#define DOS_BASE_NAME dosbase
#include <proto/dos.h>
#undef __NOLIBBASE__

/* entry points (defined below) */
ULONG BLITZ_ROUTINE date_c(struct Library *dosbase BL_REG(a6));
ULONG BLITZ_ROUTINE datestr_c(LONG days BL_REG(d0));
ULONG BLITZ_ROUTINE calcdays_c(const UBYTE *s BL_REG(d0));
void BLITZ_ROUTINE dateformat_c(UWORD f BL_REG(d0));
UWORD BLITZ_ROUTINE days_c(void);
UWORD BLITZ_ROUTINE months_c(void);
UWORD BLITZ_ROUTINE years_c(void);
ULONG BLITZ_ROUTINE weekday_c(void);
ULONG BLITZ_ROUTINE hours_c(struct Library *dosbase BL_REG(a6));
ULONG BLITZ_ROUTINE mins_c(void);
ULONG BLITZ_ROUTINE secs_c(void);

/* !libheader{#datelib,0,0,0,0} */
BLITZ_LIBHEADER(LIB_datelib, 0, BL_NORET, 0, 0);

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS0();
BLITZ_LIBS(LIB_doslib, BL_LA(6));
BLITZ_SUBS(date_c, 0, 0);
BLITZ_NAME("SystemDate", "Returns system date as number of days since 1/1/1978");

BLITZ_AFUNCTION(BL_STRING);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(datestr_c, 0, 0);
BLITZ_NAME("Date$", "days.l ;converts days since 1/1/1978 to date string");

BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(BL_STRING);
BLITZ_LIBS0();
BLITZ_SUBS(calcdays_c, 0, 0);
BLITZ_NAME("NumDays", "date$ ;converts dd/mm/yyyy to days since 1/1/1978");

BLITZ_ASTATEMENT();
BLITZ_ARGS(BL_WORD);
BLITZ_LIBS0();
BLITZ_SUBS(dateformat_c, 0, 0);
BLITZ_NAME("DateFormat", "0=dd/mm/yyyy 1=mm/dd/yyyy");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(days_c, 0, 0);
BLITZ_NAME("Days", " ;returns day calculated in last call to date$");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(months_c, 0, 0);
BLITZ_NAME("Months", " ;returns month calculated in last call to date$");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(years_c, 0, 0);
BLITZ_NAME("Years", " ;returns year calculated in last call to date$");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(weekday_c, 0, 0);
BLITZ_NAME("WeekDay", " ;returns day of week calculated in last call to date$");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS0();
BLITZ_LIBS(LIB_doslib, BL_LA(6));
BLITZ_SUBS(hours_c, 0, 0);
BLITZ_NAME("Hours", " ;gets system time and returns hour of day");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(mins_c, 0, 0);
BLITZ_NAME("Mins", " ;returns minutes calculated at last call to hour");

BLITZ_AFUNCTION(BL_WORD);
BLITZ_ARGS0();
BLITZ_LIBS0();
BLITZ_SUBS(secs_c, 0, 0);
BLITZ_NAME("Secs", " ;returns seconds calculated at last call to hour");

BLITZ_LIBFIN();

#define DAYSINQUADYR (365 * 4 + 1)                      /* 4 year cycle */
#define DAYSP1978 (DAYSINQUADYR * 494 + 366 + 365)      /* 0/0/0 to 1/1/78 */

/* register helpers (blitzhelp.h): BL_SETW/BL_SETB keep the rest of the
   register, bl_divu_w/bl_divs_w are the instructions (on overflow the
   register stays unchanged, as in modlib) */

static UWORD dateformat;                /* 0 = dd/mm/yyyy, else mm/dd/yyyy */
static ULONG totday;                    /* days since 0/0/0 of the last Date$ */
static UWORD dym[3];                    /* day, year, month of the last Date$ */

/* stamp and months in one block, as in the original: NumDays reads
   months-2 for month 0 (the low word of the tick count) and the words
   after the table for months 14/15 (the original library ends there with
   MOVEQ #0,d0:RTS). */
static struct {
    ULONG stamp[3];                     /* struct DateStamp */
    UWORD months[13];
    UWORD tail[2];
} dt = {
    { 0, 0, 0 },
    { 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 1000 },
    { 0x7000, 0x4e75 },
};

/* _hours - JSR _DateStamp(a6); minutes / 60: hours in the low word,
   minutes in the high word */
ULONG BLITZ_ROUTINE hours_c(struct Library *dosbase BL_REG(a6))
{
    DateStamp((struct DateStamp *)dt.stamp);
    return bl_divu_w(dt.stamp[1], 60);
}

/* _mins - of the last DateStamp: minutes / 60, SWAP */
ULONG BLITZ_ROUTINE mins_c(void)
{
    ULONG d0 = bl_divu_w(dt.stamp[1], 60);
    return BL_SWAP(d0);
}

/* _secs - of the last DateStamp: ticks / 50 */
ULONG BLITZ_ROUTINE secs_c(void)
{
    return bl_divu_w(dt.stamp[2], 50);
}

/* _dateformat - MOVE d0,dateformat */
void BLITZ_ROUTINE dateformat_c(UWORD f BL_REG(d0))
{
    dateformat = f;
}

/* _date - JSR _DateStamp(a6): the day count */
ULONG BLITZ_ROUTINE date_c(struct Library *dosbase BL_REG(a6))
{
    DateStamp((struct DateStamp *)dt.stamp);
    return dt.stamp[0];
}

/* two digits of the low word of r: EXT.l, DIVU #10, quotient and
   remainder + "0" (a value outside 0..99 gives other characters) */
static void put2(ULONG r)
{
    ULONG d3 = bl_divu_w(BL_EXTL(r), 10);
    *blitz_string++ = (UBYTE)(d3 + 48);
    d3 = BL_SWAP(d3);
    *blitz_string++ = (UBYTE)(d3 + 48);
}

/* _datestr - days since 1/1/1978 to "dd/mm/yyyy" at a3; d0 = length 10 */
ULONG BLITZ_ROUTINE datestr_c(LONG days BL_REG(d0))
{
    ULONG d0 = (ULONG)days + DAYSP1978, d1, d2;
    const UWORD *a0;

    totday = d0;
    d1 = bl_divs_w(d0, DAYSINQUADYR);                   /* cycles */
    d2 = (ULONG)(DAYSINQUADYR * (LONG)(WORD)d1);        /* MULS d1,d2 */
    d0 -= d2;                                           /* day in cycle */
    BL_SETW(d1, (UWORD)d1 << 2);                        /* ASL #2,d1: year */
    /* BUG(orig): BLE - day 366 stays in the leap year (32/12) */
    if ((WORD)d0 > 366) {
        BL_SETW(d0, d0 - 366);
        d0 = bl_divs_w(BL_EXTL(d0), 365);
        BL_SETW(d1, d1 + d0);
        BL_SETW(d1, d1 + 1);
        d0 = BL_SWAP(d0);                               /* day in year */
        if ((WORD)d0 > 31 + 27)
            BL_SETW(d0, d0 + 1);                        /* no Feb 29th */
    }
    /* month: first table entry > day (BPL on the word difference) */
    d2 = 0;
    a0 = dt.months + 1;
    do
        d2++;
    while ((WORD)((UWORD)d0 - *a0++) >= 0);
    BL_SETW(d0, d0 - a0[-2]);
    BL_SETW(d0, d0 + 1);
    dym[0] = d0;                                        /* MOVEM d0-d2,dym */
    dym[1] = d1;
    dym[2] = d2;
    if (dateformat) {                                   /* EXG d0,d2 */
        ULONG t = d0;
        d0 = d2;
        d2 = t;
    }
    put2(d0);
    *blitz_string++ = '/';
    put2(d2);
    *blitz_string++ = '/';
    d1 = bl_divu_w(BL_EXTL(d1), 100);
    put2(d1);
    d1 = BL_SWAP(d1);
    put2(d1);
    return 10;
}

/* _weekday - of the last Date$ (0 = Sunday).
   BUG(orig): SUBQ.w #2 - no borrow from the high word. */
ULONG BLITZ_ROUTINE weekday_c(void)
{
    ULONG d0 = totday;

    BL_SETW(d0, d0 - 2);
    d0 = bl_divu_w(d0, 32767);
    d0 = BL_SWAP(d0);
    d0 = BL_EXTL(d0);
    d0 = bl_divu_w(d0, 7);
    return BL_SWAP(d0);
}

UWORD BLITZ_ROUTINE days_c(void)   { return dym[0]; }
UWORD BLITZ_ROUTINE months_c(void) { return dym[2]; }
UWORD BLITZ_ROUTINE years_c(void)  { return dym[1]; }

/* _calcdays - "dd/mm/yyyy" (or mm/dd/yyyy) to days since 1/1/1978; 0 if
   day or month are not followed by "/".  Day and month: 1 or 2 digits,
   year: up to 4 (ends at "/" or after 4 characters, not at the end of the
   string).  "Digits" are not checked: every character counts as char-48. */
ULONG BLITZ_ROUTINE calcdays_c(const UBYTE *s BL_REG(d0))
{
    const UBYTE *a0 = s;
    ULONG d0, d1, d2 = 0, d3 = 0, d4 = 0;
    int i;

    d1 = *a0++;                                         /* day */
    if ((UWORD)d1 == '/')
        goto getm;
    BL_SETW(d1, d1 - 48);
    BL_SETB(d2, *a0++);
    if ((UWORD)d2 == '/')
        goto getm;
    BL_SETW(d2, d2 - 48);
    d1 = (UWORD)d1 * 10UL;                              /* MULU #10,d1 */
    BL_SETW(d1, d1 + d2);
    if (*a0++ != '/')
        return 0;                                       /* skipcalc */
getm:
    BL_SETW(d3, d1);
    d1 = *a0++;                                         /* month */
    if ((UWORD)d1 == '/')
        goto gety;
    BL_SETW(d1, d1 - 48);
    BL_SETB(d2, *a0++);
    if ((UWORD)d2 == '/')
        goto gety;
    BL_SETW(d2, d2 - 48);
    d1 = (UWORD)d1 * 10UL;
    BL_SETW(d1, d1 + d2);
    if (*a0++ != '/')
        return 0;
gety:
    BL_SETW(d4, d1);
    d1 = *a0++;                                         /* year */
    /* BUG(orig): "/" here jumps back to gety (month = 47) */
    if ((UWORD)d1 == '/')
        goto gety;
    BL_SETW(d1, d1 - 48);
    for (i = 0; i < 3; i++) {
        BL_SETB(d2, *a0++);
        if ((UWORD)d2 == '/')
            break;
        BL_SETW(d2, d2 - 48);
        d1 = (UWORD)d1 * 10UL;
        BL_SETW(d1, d1 + d2);
    }
    /* goty */
    if (dateformat) {                                   /* EXG d3,d4 */
        ULONG t = d3;
        d3 = d4;
        d4 = t;
    }
    d0 = 365UL * (UWORD)d1;                             /* MULU d1,d0 */
    d1 = bl_divu_w(d1, 4);
    BL_SETW(d2, d1);                                    /* leap days */
    BL_SETW(d4, d4 + d4);
    /* ADD months-2(pc,d4.w),d2 */
    BL_SETW(d2, d2 + *(const UWORD *)((const UBYTE *)dt.months - 2 + (WORD)d4));
    d0 += BL_EXTL(d2);
    d1 = BL_SWAP(d1);
    /* not a leap year and before March: one leap day less in the table */
    if ((UWORD)d1 != 0 && (WORD)((UWORD)d4 - 6) < 0)
        BL_SETW(d0, d0 + 1);                            /* BUG(orig): ADDQ.w */
    d0 += BL_EXTL(d3);
    d0 -= DAYSP1978;
    BL_SETW(d0, d0 - 1);                                /* BUG(orig): SUBQ.w */
    return d0;
}
