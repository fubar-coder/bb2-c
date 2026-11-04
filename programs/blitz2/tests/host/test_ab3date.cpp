// test_ab3date - dos.library's day number as a date (programs/blitz2/src/
// ab3date.c, the escapes \\__DATE__ and \\__DATE_GER__ of blitz2-ab3; spec
// ab3-compat/strings, design D6 of the change ab3-mode-g3): fixed days
// (1978-01-01, leap days, the century rule, 2026-10-25), every day of
// 1978 to 2500 as the successor of the day before, and the two texts.
#include <cstdint>
#include <string>
#include <snitch/snitch.hpp>
extern "C" {
#include "ab3date.h"
}

namespace {

struct date {
    uint32_t y;
    unsigned m, d;
    bool operator==(const date &) const = default;
};

date conv(uint32_t days)
{
    date r{};
    uint8_t m = 0, d = 0;

    ab3_daydate(days, &r.y, &m, &d);
    r.m = m;
    r.d = d;
    return r;
}

std::string text(uint32_t days, int ger)
{
    char buf[AB3DATE_TEXTLEN];
    unsigned n = ab3_datetext(days, ger, buf);

    REQUIRE(n < AB3DATE_TEXTLEN);
    CHECK(buf[n] == 0);
    return std::string(buf, n);
}

bool leap(uint32_t y)
{
    return (y % 4 == 0 && y % 100 != 0) || y % 400 == 0;
}

unsigned monthdays(uint32_t y, unsigned m)
{
    static const unsigned n[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    return m == 2 && leap(y) ? 29 : n[m - 1];
}

} // namespace

TEST_CASE("fixed days", "[ab3date]")
{
    CHECK(conv(0) == date{1978, 1, 1});
    CHECK(conv(364) == date{1978, 12, 31});
    CHECK(conv(365) == date{1979, 1, 1});
    // a leap year
    CHECK(conv(788) == date{1980, 2, 28});
    CHECK(conv(789) == date{1980, 2, 29});
    CHECK(conv(790) == date{1980, 3, 1});
    // 2000 is a leap year (divisible by 400), 2100 is not
    CHECK(conv(8034) == date{1999, 12, 31});
    CHECK(conv(8094) == date{2000, 2, 29});
    CHECK(conv(8095) == date{2000, 3, 1});
    CHECK(conv(44618) == date{2100, 2, 28});
    CHECK(conv(44619) == date{2100, 3, 1});
    CHECK(conv(154191) == date{2400, 2, 29});
    // the date of the tests (inject datestamp=2026-10-25)
    CHECK(conv(17829) == date{2026, 10, 25});
    CHECK(conv(2929974) == date{9999, 12, 31});
}

TEST_CASE("every day follows the one before", "[ab3date]")
{
    date prev = conv(0), cur;
    uint32_t days, firstbad = 0;

    for (days = 1; prev.y < 2500 && !firstbad; days++) {
        cur = conv(days);
        date want = prev;
        if (++want.d > monthdays(want.y, want.m)) {
            want.d = 1;
            if (++want.m > 12) {
                want.m = 1;
                want.y++;
            }
        }
        if (!(cur == want))
            firstbad = days;
        prev = cur;
    }
    CHECK(firstbad == 0u);
    CHECK(prev == date{2500, 1, 1});
}

TEST_CASE("the largest day number", "[ab3date]")
{
    // DateStamp's ds_Days is a LONG: its largest value still gives a date
    date d = conv(0x7fffffffUL);

    CHECK(d.y > 5000000);
    CHECK(d.m >= 1);
    CHECK(d.m <= 12);
    CHECK(d.d >= 1);
    CHECK(d.d <= monthdays(d.y, d.m));
    CHECK(text(0x7fffffffUL, 0).size() <= AB3DATE_TEXTLEN - 1);
}

TEST_CASE("the texts of the escapes", "[ab3date]")
{
    // \\__DATE__: mm.dd.yyyy, \\__DATE_GER__: dd.mm.yyyy
    CHECK(text(17829, 0) == "10.25.2026");
    CHECK(text(17829, 1) == "25.10.2026");
    // two digits for month and day
    CHECK(text(0, 0) == "01.01.1978");
    CHECK(text(0, 1) == "01.01.1978");
    CHECK(text(8094, 0) == "02.29.2000");
    CHECK(text(8094, 1) == "29.02.2000");
}
