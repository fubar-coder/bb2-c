/*
 * datelib - command library 78: dates as days since 1/1/1978 and the
 * system time (SystemDate, Date$, NumDays, DateFormat, Days, Months,
 * Years, WeekDay, Hours, Mins, Secs).
 *
 * C translation of bblibsrc/datelib (the CD binary matches the source).
 * Dates are counted from day 0 = 1/1/0000 in 4 year cycles of 1461 days
 * (every 4th year a leap year), the month table is the cumulative one of
 * a leap year.  The string format is dd/mm/yyyy or (DateFormat 1)
 * mm/dd/yyyy.
 *
 * The routines do their arithmetic with word operations and DIVU/DIVS;
 * to stay bit-exact also for invalid input, the C code below follows the
 * register operations of the original: values are kept as 32 bit
 * "registers", word/byte operations change only the low word/byte
 * (SETW/SETB), DIVU/DIVS are the instructions (overflow leaves the
 * register unchanged).
 *
 * Fixed (intended deviation, #120): Date$ of day 366 of a cycle (1/1 of
 *   the year after a leap year, e.g. 1/1/1981): the original showed it as
 *   32/12 of the leap year (CMP #366: BLE instead of BLT); here BLT.
 * Fixed (intended deviation, #1001): the 4 year cycles make 1800, 1900,
 *   2100, 2200, ... leap years, so the original's dates were right only
 *   from 1/3/1900 to 28/2/2100 (a day off for each century year that is
 *   not divisible by 400).  Date$ and NumDays now correct the day count by
 *   these days (gregskips): the dates are Gregorian, as those of
 *   AmigaDOS; from 1/3/1900 to 28/2/2100 nothing changes.
 * Kept on purpose (BUG(orig)):
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

/**
 * @brief Hours: get the system time, the hour of the day.
 *
 * _hours: JSR _DateStamp(a6) into the library's DateStamp (also for Mins
 * and Secs), then minutes / 60 (DIVU).
 *
 * @param dosbase  a6: dos.library
 * @return d0: the hours in the low word, the minutes in the high word
 */
ULONG BLITZ_ROUTINE hours_c(struct Library *dosbase BL_REG(a6))
{
    DateStamp(&dt.stamp);
    return bl_divu_w(dt.stamp.ds_Minute, 60);
}

/**
 * @brief Mins: the minutes of the time of the last Hours call.
 *
 * _mins: of the last DateStamp (no DateStamp of its own; documented, not
 * a bug): minutes / 60, SWAP.
 *
 * @return d0: the minutes in the low word, the hours in the high word
 */
ULONG BLITZ_ROUTINE mins_c(void)
{
    ULONG hourmin = bl_divu_w(dt.stamp.ds_Minute, 60);
    return BL_SWAP(hourmin);
}

/**
 * @brief Secs: the seconds of the time of the last Hours call.
 *
 * _secs: of the last DateStamp (no DateStamp of its own): ticks / 50.
 *
 * @return d0: the seconds in the low word, the remaining ticks in the
 *         high word
 */
ULONG BLITZ_ROUTINE secs_c(void)
{
    return bl_divu_w(dt.stamp.ds_Tick, 50);
}

/**
 * @brief DateFormat 0|1: the string format of Date$ and NumDays.
 *
 * _dateformat: MOVE d0,dateformat.
 *
 * @param f  d0.w: 0 = dd/mm/yyyy, otherwise mm/dd/yyyy
 */
void BLITZ_ROUTINE dateformat_c(UWORD f BL_REG(d0))
{
    dateformat = f;
}

/**
 * @brief SystemDate: the system date as days since 1/1/1978.
 *
 * _date: JSR _DateStamp(a6) into the library's DateStamp.
 *
 * @param dosbase  a6: dos.library
 * @return d0: ds_Days
 */
ULONG BLITZ_ROUTINE date_c(struct Library *dosbase BL_REG(a6))
{
    DateStamp(&dt.stamp);
    return dt.stamp.ds_Days;
}

/* n / d for a 32 bit n (two DIVU.w: the quotient may need 32 bits);
   *rem = the remainder */
static ULONG divu32(ULONG n, UWORD d, ULONG *rem)
{
    ULONG hi = bl_divu_w(n >> 16, d);
    ULONG lo = bl_divu_w((hi & 0xffff0000UL) | (UWORD)n, d);

    *rem = lo >> 16;
    return hi << 16 | (UWORD)lo;
}

/* Fixed (#1001): the century leap days of the 4 year cycles that the
   Gregorian calendar does not have (years divisible by 100, not by 400)
   up to the day days.l (days since 1/1/1978, Gregorian), less the 15 up
   to 1/1/1978: what Date$ adds to the day count of the cycles.  0 from
   1/3/1900 to 28/2/2100.  z = the days since 1/3/0000 (Gregorian), in
   400 year eras of 146097 = 27 * 5411 days (floor division); in an era
   the first three centuries of 36524 days each end with a skipped day. */
static LONG gregskips(LONG days)
{
    ULONG z = (ULONG)days + 722390;
    ULONG r1, r2, era, doe, cent;

    if ((LONG)z >= 0) {
        era = divu32(divu32(z, 27, &r1), 5411, &r2);
        doe = bl_mulu_w((UWORD)r2, 27) + r1;
    } else {                                    /* floor: z = -n - 1 */
        era = ~divu32(divu32(~z, 27, &r1), 5411, &r2);
        doe = 146096 - (bl_mulu_w((UWORD)r2, 27) + r1);
    }
    cent = (UWORD)bl_divu_w(doe, 36524);
    if (cent > 3)
        cent = 3;                               /* 29/2 of the 400th year */
    return (LONG)(era + era + era + cent) - 15;
}

/* Fixed (#1001): the same for NumDays from the year of the date and
   whether it is before March: the skipped days up to the year from March
   on (year - 1 before March), less 15. */
static LONG gregskips_year(UWORD year, int early)
{
    LONG y = (LONG)year - (early ? 1 : 0);

    if (y < 0)
        return -15;
    return (LONG)(UWORD)bl_divu_w((ULONG)y, 100)
         - (LONG)(UWORD)bl_divu_w((ULONG)y, 400) - 15;
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

/**
 * @brief Date$(days.l): days since 1/1/1978 as a date string.
 *
 * _datestr: "dd/mm/yyyy" (or mm/dd/yyyy after DateFormat 1) at a3; keeps
 * day, year and month for Days, Years, Months and the day count for
 * WeekDay.  Dates are counted from day 0 = 1/1/0000 in 4 year cycles of
 * 1461 days.  d1 holds the 4 year cycles first, d2 the days of the
 * cycles; for mm/dd/yyyy day and month are exchanged (EXG) before the
 * output.  Values outside 0..99 per two digits give other characters.
 * Fixed (#120): day 366 of a cycle (1/1 of the year after a leap year,
 * e.g. 1/1/1981) is the first day of the next year; the original showed
 * it as 32/12 of the leap year (CMP #366: BLE instead of BLT).
 * Fixed (#1001): the day count of the cycles is corrected by the century
 * leap days the Gregorian calendar does not have (gregskips): the date is
 * Gregorian (the original showed 29/2/2100 for 1/3/2100).  WeekDay keeps
 * the uncorrected count (the weekday follows from the days alone).
 *
 * @param days  d0: the days since 1/1/1978
 * @return d0: the length of the string, 10
 */
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
    day += (ULONG)gregskips(days);                      /* fixed (#1001) */
    cycles = bl_divs_w(day, DAYSINQUADYR);
    cycdays = (ULONG)(DAYSINQUADYR * (LONG)(WORD)cycles); /* MULS d1,d2 */
    day -= cycdays;                                     /* day in cycle */
    year = cycles;
    BL_SETW(year, (UWORD)cycles << 2);                  /* ASL #2,d1: year */
    /* fixed (#120): BLT - the original's BLE kept day 366 in the leap
       year (32/12) */
    if ((WORD)day >= 366) {
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

/**
 * @brief WeekDay: the day of the week of the last Date$ (0 = Sunday).
 *
 * BUG(orig) (#121): SUBQ.w #2 on the day count - no borrow from the high
 * word.
 *
 * @return d0.w: the day of the week, 0..6
 */
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

/**
 * @brief Days: the day of the month of the last Date$.
 * @return d0.w: the day
 */
UWORD BLITZ_ROUTINE days_c(void)   { return dym.day; }
/**
 * @brief Months: the month of the last Date$.
 * @return d0.w: the month
 */
UWORD BLITZ_ROUTINE months_c(void) { return dym.month; }
/**
 * @brief Years: the year of the last Date$.
 * @return d0.w: the year
 */
UWORD BLITZ_ROUTINE years_c(void)  { return dym.year; }

/**
 * @brief NumDays(date$): a date string as days since 1/1/1978.
 *
 * _calcdays: "dd/mm/yyyy" (or mm/dd/yyyy after DateFormat 1).  Day and
 * month: 1 or 2 digits, year: up to 4 (ends at "/" or after 4
 * characters, not at the end of the string).  "Digits" are not checked:
 * every character counts as char-48.  d1 is the field being read, later
 * the year / 4; d2 the character, later the leap days; d3 and d4 hold
 * the first and second field, after the EXG for mm/dd/yyyy the day and
 * the month.
 * BUG(orig) (#122): ADDQ.w #1 / SUBQ.w #1 on the day count (no
 * carry/borrow), e.g. NumDays("31/12/1977") = 65535, not -1.
 * BUG(orig) (#123): a "/" as first character of a field becomes the
 * value 47.
 * BUG(orig) (#124): month 0 reads the low word of the tick count of the
 * last DateStamp, months 14/15 the two words after the table.
 * BUG(orig) (#125): a 2 digit year reads past the end of the string.
 * Fixed (#1001): the days of the cycles are corrected by the century leap
 * days the Gregorian calendar does not have (gregskips_year); the
 * original's NumDays("1/3/2100") was one day too high.
 *
 * @param s  d0: the date string
 * @return d0: the days since 1/1/1978; 0 if day or month are not
 *         followed by "/"
 */
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
    /* fixed (#1001): the Gregorian century rule */
    total -= (ULONG)gregskips_year((UWORD)num, (WORD)((UWORD)moff - 6) < 0);
    BL_SETW(total, total - 1);                          /* BUG(orig) (#122): SUBQ.w */
    return total;
}
