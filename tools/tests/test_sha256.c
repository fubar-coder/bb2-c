/*
 * test_sha256.c - sha256 against the FIPS 180-4 examples and pieces of
 * every size (block boundaries, padding into a second block).
 */
#include "testutil.h"
#include "sha256.h"

static int is(const char *data, size_t len, const char *want)
{
    char hex[65];

    sha256_hex_of(data, len, hex);
    return !strcmp(hex, want);
}

int main(void)
{
    static char a[1000000];
    char hex[65], whole[65];
    struct sha256 s;
    size_t i, step;

    CHECK(is("", 0, "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"));
    CHECK(is("abc", 3, "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"));
    CHECK(is("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq", 56,
             "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1"));
    memset(a, 'a', sizeof a);
    CHECK(is(a, sizeof a, "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0"));

    /* the same text in pieces of 1..130 bytes */
    sha256_hex_of(a, 1000, whole);
    for (step = 1; step <= 130; step++) {
        sha256_init(&s);
        for (i = 0; i < 1000; i += step)
            sha256_update(&s, a + i, i + step <= 1000 ? step : 1000 - i);
        sha256_hex(&s, hex);
        CHECK(!strcmp(hex, whole));
    }
    return failures != 0;
}
