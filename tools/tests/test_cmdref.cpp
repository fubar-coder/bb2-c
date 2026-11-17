// test_cmdref - the JSON of cmdref on libraries built in memory: a
// command library, an AmigaOS call library with and without the name of
// the library it opens, escaping, and broken inputs.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <snitch/snitch.hpp>
#include "testutil.hpp"
extern "C" {
#include "cmdref.h"
}

/* header ($30 bytes): MOVEQ #0,d0:RTS or 4 zero bytes, libnum */
static void header(buf &b, int oslib, uint16_t libnum)
{
    put32(b, oslib ? 0 : 0x70004E75);
    put16(b, libnum);
    while (b.size() < 0x30)
        put8(b, 0);
}

static void subs(buf &b)
{
    put16(b, 0);                        /* no libs */
    put32(b, 0x80); put32(b, 0); put32(b, 0);
}

/* name block with its lead $FFFF; name and help padded to even */
static void name(buf &b, int lead, const char *n, const char *h)
{
    if (lead)
        put16(b, 0xFFFF);
    put32(b, 0);
    put16(b, 0);
    putbytes(b, n, strlen(n) + 1);
    putbytes(b, h, strlen(h) + 1);
    if (b.size() & 1)
        put8(b, 0);
}

/* the JSON of the library in t; NULL (and the message in err) if it
   does not read */
static char *json(const buf &t, const char *group, const char *nm, char *err, size_t len)
{
    struct hunk h = { HUNK_CODE, t.data(), (uint32_t)t.size(), 0, NULL, NULL, 0, NULL, 0, 0 };
    struct blitzlib lib;
    const char *opens;
    FILE *f;
    char *s;
    long n;

    if (blitzlib_read(&h, &lib, err, len))
        return NULL;
    if (cmdref_opens(&lib, &opens, err, len)) {
        blitzlib_free(&lib);
        return NULL;
    }
    f = tmpfile();
    REQUIRE(f != nullptr);
    REQUIRE(cmdref_write(f, &lib, group, nm, opens) == 0);
    n = ftell(f);
    rewind(f);
    s = static_cast<char *>(malloc((size_t)n + 1));
    REQUIRE(s != nullptr);
    REQUIRE(fread(s, 1, (size_t)n, f) == (size_t)n);
    s[n] = 0;
    fclose(f);
    blitzlib_free(&lib);
    return s;
}

static const char expect_cmd[] =
    "{\n"
    "  \"name\": \"testlib\",\n"
    "  \"group\": \"acidlibs\",\n"
    "  \"number\": 100,\n"
    "  \"object\": {\"name\": \"Thing\", \"maximum\": 20},\n"
    "  \"entries\": [\n"
    "    {\n"
    "      \"name\": \"Fn\",\n"
    "      \"kind\": \"function\",\n"
    "      \"help\": \"(a,b\\\"\\u00e4)\",\n"
    "      \"result\": \"long\",\n"
    "      \"forms\": [\n"
    "        {\"args\": [{\"type\": \"word\", \"code\": 2}, {\"type\": \"string\", \"code\": 7}]},\n"
    "        {\"args\": [{\"type\": \"long\", \"code\": 3}, {\"type\": \"any\", \"code\": 8}, "
    "{\"type\": \"long\", \"code\": 131, \"flags\": [\"address\"]}], "
    "\"repeat\": {\"from\": 1, \"count\": 1}}\n"
    "      ]\n"
    "    },\n"
    "    {\n"
    "      \"name\": \"Peek\",\n"
    "      \"kind\": \"function\",\n"
    "      \"typed\": true,\n"
    "      \"help\": \"[.Type](Address)\",\n"
    "      \"result\": \"suffix\",\n"
    "      \"forms\": [\n"
    "        {\"args\": [{\"type\": \"array\", \"code\": 120, \"flags\": "
    "[\"arraytype\", \"arrayend\", \"stack\"]}]}\n"
    "      ]\n"
    "    },\n"
    "    {\n"
    "      \"name\": \"St\",\n"
    "      \"kind\": \"statement\",\n"
    "      \"help\": \"\",\n"
    "      \"forms\": [\n"
    "        {\"args\": []}\n"
    "      ]\n"
    "    },\n"
    "    {\n"
    "      \"name\": \"Thing\",\n"
    "      \"kind\": \"object\",\n"
    "      \"help\": \"x\"\n"
    "    }\n"
    "  ]\n"
    "}\n";

static const char expect_os[] =
    "{\n"
    "  \"name\": \"doslib\",\n"
    "  \"group\": \"deflibs\",\n"
    "  \"number\": 252,\n"
    "  \"opens\": \"dos.library\",\n"
    "  \"entries\": [\n"
    "    {\n"
    "      \"name\": \"Open_\",\n"
    "      \"kind\": \"oscall\",\n"
    "      \"help\": \"(name,accessMode)\",\n"
    "      \"base\": 252,\n"
    "      \"offset\": -30,\n"
    "      \"registers\": [\"d1\", \"a2\"]\n"
    "    },\n"
    "    {\n"
    "      \"name\": \"Input_\",\n"
    "      \"kind\": \"oscall\",\n"
    "      \"help\": \"\",\n"
    "      \"base\": 252,\n"
    "      \"offset\": -54,\n"
    "      \"registers\": []\n"
    "    }\n"
    "  ]\n"
    "}\n";

/* an AmigaOS call library: two calls, a nullsub, then code with the
   strings in `code` */
static buf oslib(const char *code, size_t len)
{
    buf t;

    header(t, 1, 252);
    put16(t, 6); put32(t, 0);
    put16(t, 252); put16(t, (uint16_t)-30);
    put8(t, 0x01); put8(t, 0x12); put8(t, 0xFF); put8(t, 0);
    name(t, 0, "Open_", "(name,accessMode)");
    put16(t, 6); put32(t, 0);
    put16(t, 252); put16(t, (uint16_t)-54);
    put8(t, 0xFF); put8(t, 0);
    name(t, 0, "Input_", "");
    put16(t, 0); put32(t, 0);         /* nullsub (init) */
    subs(t);
    put16(t, 0xFFFF);
    put32(t, 0);                       /* no !libfin */
    put16(t, 0x4E75);
    putbytes(t, code, len);
    return t;
}

TEST_CASE("cmdref: checks", "[cmdref]")
{
    buf t;
    char err[300], *s;
    uint32_t objname;

    /* a command library with an object */
    header(t, 0, 100);
    put16(t, 0x0302); put32(t, 0);
    put16(t, 0x0002); put8(t, 2); put8(t, 7);
    subs(t);
    put16(t, 0x1103); put8(t, 3); put8(t, 8); put8(t, 0x83); put8(t, 0);
    subs(t);
    name(t, 1, "Fn", "(a,b\"\xE4)");
    put16(t, 8 + 2); put32(t, 0);     /* !ufunction */
    put16(t, 0x0001); put8(t, 0x78); put8(t, 0);
    subs(t);
    name(t, 1, "Peek", "[.Type](Address)");
    put16(t, 1); put32(t, 0);
    put16(t, 0x0000);
    subs(t);
    name(t, 1, "St", "");
    put16(t, 8); put32(t, 0);         /* dumtoke */
    put32(t, 0); put16(t, 0);
    objname = (uint32_t)t.size();
    putbytes(t, "Thing\0x\0", 8);
    put16(t, 0xFFFF);
    put32(t, objname);
    put32(t, 0); put32(t, 0); put32(t, 0); put32(t, 0);
    put16(t, 0); put16(t, 20); put16(t, 3);
    put16(t, 0x4E75);
    /* a library name in a command library is not an "opens" */
    putbytes(t, "x.library\0", 10);
    s = json(t, "acidlibs", "testlib", err, sizeof err);
    CHECK(s && !strcmp(s, expect_cmd));
    if (s && strcmp(s, expect_cmd))
        fprintf(stderr, "got:\n%s", s);
    free(s);

    /* an AmigaOS call library: the library name among other bytes */
    t = oslib("\x01\x02" "dos.library\0\0abc\0", 19);
    s = json(t, "deflibs", "doslib", err, sizeof err);
    CHECK(s && !strcmp(s, expect_os));
    if (s && strcmp(s, expect_os))
        fprintf(stderr, "got:\n%s", s);
    free(s);

    /* no init, no name: no "opens" */
    t = oslib("abc\0", 4);
    s = json(t, "deflibs", "doslib2", err, sizeof err);
    CHECK(s && !strstr(s, "opens") && strstr(s, "\"name\": \"doslib2\""));
    free(s);

    /* a resource and a gadget are names too; two of them are an error */
    t = oslib("ciaa.resource\0gadgets/colorwheel.gadget\0", 40);
    s = json(t, "deflibs", "x", err, sizeof err);
    CHECK(!s && strstr(err, "two library names") && strstr(err, "ciaa.resource"));
    free(s);

    /* a name not followed by its 0 inside the code is none */
    t = oslib("dos.library", 11);
    s = json(t, "deflibs", "x", err, sizeof err);
    CHECK(s && !strstr(s, "opens"));
    free(s);

    /* broken: the table runs past the end of the code */
    t.clear();
    header(t, 0, 100);
    put16(t, 1); put32(t, 0);
    put16(t, 0x0005); put8(t, 2);
    s = json(t, "acidlibs", "x", err, sizeof err);
    CHECK(!s && strstr(err, "past the end"));

}
