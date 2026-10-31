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
 *     is shown as 32/12 of the leap year (CMP #366: BLE instead of BLT). (#120)
 *   - WeekDay: SUBQ.w #2 on the day count (no borrow into the high word). (#121)
 *   - NumDays: ADDQ.w #1 / SUBQ.w #1 on the day count (no carry/borrow),
 *     e.g. NumDays("31/12/1977") = 65535, not -1. (#122)
 *   - NumDays: a 2 digit year reads past the end of the string (the year
 *     ends only at "/" or after 4 characters) (#125); a "/" as first
 *     character of a field becomes the value 47 (#123); month 0 reads the
 *     low word of the tick count of the last DateStamp (word before the
 *     month table),
 *     months 14/15 read the two words after the table (MOVEQ #0,d0:RTS at
 *     the end of the original library, reproduced below) (#124).
 * Not a bug (documented): Mins/Secs return the values of the last Hours
 * call (no DateStamp of their own).
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
static struct {                        /* the last Date$ (dym: MOVEM d0-d2) */
    UWORD day, year, month;
} dym;
static_assert(sizeof dym == 3 * 2);

/* stamp and months in one block, as in the original: NumDays reads
   months-2 for month 0 (the low word of the tick count) and the words
   after the table for months 14/15 (the original library ends there with
   MOVEQ #0,d0:RTS). */
static struct {
    struct DateStamp stamp;
    UWORD months[13];
    UWORD tail[2];
} dt = {
    { 0, 0, 0 },
    { 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 1000 },
    { 0x7000, 0x4e75 },
};
static_assert(sizeof dt == 42);

/* _hours - JSR _DateStamp(a6); minutes / 60: hours in the low word,
   minutes in the high word */
ULONG BLITZ_ROUTINE hours_c(struct Library *dosbase BL_REG(a6))
{
    DateStamp(&dt.stamp);
    return bl_divu_w(dt.stamp.ds_Minute, 60);
}

/* _mins - of the last DateStamp: minutes / 60, SWAP */
ULONG BLITZ_ROUTINE mins_c(void)
{
    ULONG hourmin = bl_divu_w(dt.stamp.ds_Minute, 60);
    return BL_SWAP(hourmin);
}

/* _secs - of the last DateStamp: ticks / 50 */
ULONG BLITZ_ROUTINE secs_c(void)
{
    return bl_divu_w(dt.stamp.ds_Tick, 50);
}

/* _dateformat - MOVE d0,dateformat */
void BLITZ_ROUTINE dateformat_c(UWORD f BL_REG(d0))
{
    dateformat = f;
}

/* _date - JSR _DateStamp(a6): the day count */
ULONG BLITZ_ROUTINE date_c(struct Library *dosbase BL_REG(a6))
{
    DateStamp(&dt.stamp);
    return dt.stamp.ds_Days;
}

/* two digits of the low word of r: EXT.l, DIVU #10, quotient and
   remainder + "0" (a value outside 0..99 gives other characters) */
static void put2(ULONG r)
{
    ULONG digits = bl_divu_w(BL_EXTL(r), 10);     /* d3 */
    *blitz_string++ = (UBYTE)(digits + 48);
    digits = BL_SWAP(digits);
    *blitz_string++ = (UBYTE)(digits + 48);
}

/* _datestr - days since 1/1/1978 to "dd/mm/yyyy" at a3; d0 = length 10.
   d1 holds the 4 year cycles first, d2 the days of the cycles; for
   mm/dd/yyyy day and month are exchanged (EXG) before the output */
ULONG BLITZ_ROUTINE datestr_c(LONG days BL_REG(d0))
{
    ULONG day = (ULONG)days + DAYSP1978;                /* d0 */
    ULONG cycles;                                       /* d1: DIVS result */
    ULONG year;                                         /* d1 */
    ULONG cycdays;                                      /* d2 */
    ULONG month;                                        /* d2 */
    ULONG century;                                      /* d1: DIVU result */
    const UWORD *entry;

    totday = day;
    cycles = bl_divs_w(day, DAYSINQUADYR);
    cycdays = (ULONG)(DAYSINQUADYR * (LONG)(WORD)cycles); /* MULS d1,d2 */
    day -= cycdays;                                     /* day in cycle */
    year = cycles;
    BL_SETW(year, (UWORD)cycles << 2);                  /* ASL #2,d1: year */
    /* BUG(orig) (#120): BLE - day 366 stays in the leap year (32/12) */
    if ((WORD)day > 366) {
        BL_SETW(day, day - 366);
        day = bl_divs_w(BL_EXTL(day), 365);
        BL_SETW(year, year + day);
        BL_SETW(year, year + 1);
        day = BL_SWAP(day);                             /* day in year */
        if ((WORD)day > 31 + 27)
            BL_SETW(day, day + 1);                      /* no Feb 29th */
    }
    /* month: first table entry > day (BPL on the word difference) */
    month = 0;
    entry = dt.months + 1;
    do
        month++;
    while ((WORD)((UWORD)day - *entry++) >= 0);
    BL_SETW(day, day - entry[-2]);
    BL_SETW(day, day + 1);
    dym.day = day;                                      /* MOVEM d0-d2,dym */
    dym.year = year;
    dym.month = month;
    if (dateformat) {                                   /* EXG d0,d2 */
        ULONG saved = day;
        day = month;
        month = saved;
    }
    put2(day);
    *blitz_string++ = '/';
    put2(month);
    *blitz_string++ = '/';
    century = bl_divu_w(BL_EXTL(year), 100);
    put2(century);
    put2(BL_SWAP(century));
    return 10;
}

/* _weekday - of the last Date$ (0 = Sunday).
   BUG(orig) (#121): SUBQ.w #2 - no borrow from the high word. */
ULONG BLITZ_ROUTINE weekday_c(void)
{
    ULONG value = totday;                         /* d0 */

    BL_SETW(value, value - 2);
    value = bl_divu_w(value, 32767);
    value = BL_SWAP(value);
    value = BL_EXTL(value);
    value = bl_divu_w(value, 7);
    return BL_SWAP(value);
}

UWORD BLITZ_ROUTINE days_c(void)   { return dym.day; }
UWORD BLITZ_ROUTINE months_c(void) { return dym.month; }
UWORD BLITZ_ROUTINE years_c(void)  { return dym.year; }

/* _calcdays - "dd/mm/yyyy" (or mm/dd/yyyy) to days since 1/1/1978; 0 if
   day or month are not followed by "/".  Day and month: 1 or 2 digits,
   year: up to 4 (ends at "/" or after 4 characters, not at the end of the
   string).  "Digits" are not checked: every character counts as char-48.
   d1 is the field being read, later the year / 4; d2 the character,
   later the leap days; d3 and d4 hold the first and second field, after
   the EXG for mm/dd/yyyy the day and the month. */
ULONG BLITZ_ROUTINE calcdays_c(const UBYTE *s BL_REG(d0))
{
    const UBYTE *cur = s;                               /* a0 */
    ULONG total;                                        /* d0 */
    ULONG num;                                          /* d1: the field */
    ULONG quads;                                        /* d1: DIVU year by 4 */
    ULONG digit = 0;                                    /* d2: the character */
    ULONG leap;                                         /* d2: the leap days */
    ULONG field1 = 0, field2 = 0;                       /* d3 d4 */
    ULONG day, month;                                   /* d3 d4 after the EXG */
    ULONG moff;                                         /* d4: month table offset */
    int i;

    num = *cur++;                                       /* day */
    if ((UWORD)num == '/')
        goto getm;
    BL_SETW(num, num - 48);
    BL_SETB(digit, *cur++);
    if ((UWORD)digit == '/')
        goto getm;
    BL_SETW(digit, digit - 48);
    num = (UWORD)num * 10UL;                            /* MULU #10,d1 */
    BL_SETW(num, num + digit);
    if (*cur++ != '/')
        return 0;                                       /* skipcalc */
getm:
    BL_SETW(field1, num);
    num = *cur++;                                       /* month */
    if ((UWORD)num == '/')
        goto gety;
    BL_SETW(num, num - 48);
    BL_SETB(digit, *cur++);
    if ((UWORD)digit == '/')
        goto gety;
    BL_SETW(digit, digit - 48);
    num = (UWORD)num * 10UL;
    BL_SETW(num, num + digit);
    if (*cur++ != '/')
        return 0;
gety:
    BL_SETW(field2, num);
    num = *cur++;                                       /* year */
    /* BUG(orig) (#123): "/" here jumps back to gety (month = 47) */
    if ((UWORD)num == '/')
        goto gety;
    BL_SETW(num, num - 48);
    for (i = 0; i < 3; i++) {
        BL_SETB(digit, *cur++);
        if ((UWORD)digit == '/')
            break;
        BL_SETW(digit, digit - 48);
        num = (UWORD)num * 10UL;
        BL_SETW(num, num + digit);
    }
    /* goty */
    if (dateformat) {                                   /* EXG d3,d4 */
        day = field2;
        month = field1;
    } else {
        day = field1;
        month = field2;
    }
    total = 365UL * (UWORD)num;                         /* MULU d1,d0 */
    quads = bl_divu_w(num, 4);
    leap = digit;
    BL_SETW(leap, quads);                               /* leap days */
    moff = month;
    BL_SETW(moff, month + month);
    /* ADD months-2(pc,d4.w),d2 */
    BL_SETW(leap, leap + *(const UWORD *)((const UBYTE *)dt.months - 2 + (WORD)moff));
    total += BL_EXTL(leap);
    /* not a leap year (the remainder, SWAP d1) and before March: one
       leap day less in the table */
    if ((UWORD)BL_SWAP(quads) != 0 && (WORD)((UWORD)moff - 6) < 0)
        BL_SETW(total, total + 1);                      /* BUG(orig) (#122): ADDQ.w */
    total += BL_EXTL(day);
    total -= DAYSP1978;
    BL_SETW(total, total - 1);                          /* BUG(orig) (#122): SUBQ.w */
    return total;
}
