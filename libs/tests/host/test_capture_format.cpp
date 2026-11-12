// test_capture_format - the case writer of the library test programs
// (libs/tests/capture.c; design D4 and D6 of the openspec change
// libs-tests-capture) with its files in memory (capture_env.h below): the
// text written for known cases - every value format, a 64-byte and a
// 65-byte buffer, the same check twice, a key with | TAB \ and a
// non-printable byte, block cases, LIBS_CAP_DUMP of a case and of a
// block - and the faults.
#include <cstdint>
#include <cstdio>
#include <map>
#include <string>
#include <vector>
#include <snitch/snitch.hpp>
extern "C" {
#include "capture.h"
#include "capture_env.h"
}

namespace {

std::map<std::string, std::string> files;   // closed files by path
std::map<std::string, std::string> open_files;
std::vector<std::string> dirs, failures;
std::string dumpvar;
bool have_dumpvar, refuse_create;

uint32_t fnv(const std::string &s)
{
    uint32_t h = 0x811c9dc5u;
    for (unsigned char c : s)
        h = (h ^ c) * 0x01000193u;
    return h;
}

std::string hex8(uint32_t v)
{
    char b[16];
    std::snprintf(b, sizeof b, "%08lX", (unsigned long)v);
    return b;
}

void reset()
{
    files.clear();
    open_files.clear();
    dirs.clear();
    failures.clear();
    have_dumpvar = refuse_create = false;
}

} // namespace

extern "C" {

void *cap_env_create(const char *path)
{
    if (refuse_create)
        return nullptr;
    auto it = open_files.emplace(path, std::string()).first;
    return (void *)&*it;
}

void cap_env_mkdir(const char *path) { dirs.push_back(path); }

int cap_env_write(void *f, const char *p, uint32_t n)
{
    auto *e = static_cast<std::pair<const std::string, std::string> *>(f);
    e->second.append(p, n);
    return 0;
}

void cap_env_close(void *f)
{
    auto *e = static_cast<std::pair<const std::string, std::string> *>(f);
    files[e->first] = e->second;
    open_files.erase(e->first);
}

int cap_env_getvar(const char *name, char *buf, uint32_t size)
{
    if (std::string(name) != "LIBS_CAP_DUMP" || !have_dumpvar)
        return -1;
    std::snprintf(buf, size, "%s", dumpvar.c_str());
    return (int)dumpvar.size();
}

void cap_env_find_prog(void) {}

// the "test program": $1000-$1FFF
int cap_env_is_prog(uint32_t v) { return v >= 0x1000 && v < 0x2000; }

void bl_test_check(const char *what, int ok)
{
    if (!ok)
        failures.push_back(what);
}

} // extern "C"

TEST_CASE("every value format and the key", "[capture]")
{
    reset();
    unsigned char b64[64], b65[65];
    for (int i = 0; i < 65; i++) {
        b65[i] = (unsigned char)i;
        if (i < 64)
            b64[i] = (unsigned char)i;
    }
    std::string s65(reinterpret_cast<const char *>(b65), 65);
    std::string hex64;
    for (int i = 0; i < 64; i++) {
        char h[4];
        std::snprintf(h, sizeof h, "%02X", i);
        hex64 += h;
    }
    // a record holding a pointer into the test program at offset 2
    const unsigned char rec[8] = {0xAA, 0xBB, 0x00, 0x00, 0x10, 0x20, 0xCC, 0xDD};

    cap_begin("TEST:captures", "unit");
    cap_num("no routine", nullptr, 7);
    cap_routine("rtn");
    cap_num("finit d0", "x=1", 0x12345678);
    cap_num("finit d0", "x=1", 0);
    cap_num("finit d0", "x=2", 1);
    cap_num("a|b\tc\\d\x01", "v|\t\xff", 0xFFFFFFFF);
    cap_bytes("buf", "64", b64, 64);
    cap_bytes("buf", "65", b65, 65);
    cap_bytes("buf", "0", b64, 0);
    cap_ptrbytes("rec", nullptr, rec, 8);
    cap_str("s", nullptr, "a\"b\\c\td\x80");
    cap_str("s", "null", nullptr);
    cap_ptr("p", "null", 0, "base", 0x8000);
    cap_ptr("p", "prog", 0x1abc, "base", 0x8000);
    cap_ptr("p", "above", 0x8010, "base", 0x8000);
    cap_ptr("p", "below", 0x7ff0, "base", 0x8000);
    cap_ptr("p", "no base", 0x40001234, nullptr, 0);
    cap_regs("regs", nullptr, 0);
    cap_regs("regs", nullptr, (1u << 3) | (1u << 9) | (1u << 14) | 0x8000);
    cap_not_captured("wheel", "Encrypt", "Rnd\\hw\ttiming");
    cap_routine("x|y");
    cap_num("finit d0", "x=1", 5);
    CHECK(cap_end() == 0);
    CHECK(failures.empty());
    CHECK(files.size() == 1);

    const std::string want = std::string("# capture unit 1\n")
        + "|no routine||1\t$00000007\n"
        + "rtn|finit d0|x=1|1\t$12345678\n"
        + "rtn|finit d0|x=1|2\t$00000000\n"
        + "rtn|finit d0|x=2|1\t$00000001\n"
        + "rtn|a\\|b\\tc\\\\d\\x01|v\\|\\t\\xFF|1\t$FFFFFFFF\n"
        + "rtn|buf|64|1\tbytes 64 " + hex64 + "\n"
        + "rtn|buf|65|1\tbytes 65 fnv " + hex8(fnv(s65)) + "\n"
        + "rtn|buf|0|1\tbytes 0\n"
        + "rtn|rec||1\tbytes 8 AABB50524F47CCDD\n"
        + "rtn|s||1\t\"a\\\"b\\\\c\\x09d\\x80\"\n"
        + "rtn|s|null|1\tnull\n"
        + "rtn|p|null|1\t$00000000\n"
        + "rtn|p|prog|1\tprog\n"
        + "rtn|p|above|1\tbase+$10\n"
        + "rtn|p|below|1\tbase-$10\n"
        + "rtn|p|no base|1\t$40001234\n"
        + "rtn|regs||1\tkept\n"
        + "rtn|regs||2\tlost d3 a1 a6\n"
        + "rtn|wheel|Encrypt|1\tnot captured: Rnd\\\\hw\\x09timing\n"
        + "x\\|y|finit d0|x=1|1\t$00000005\n";
    CHECK(files["TEST:captures/unit.txt"] == want);
}

TEST_CASE("an output without cases; the directory joins", "[capture]")
{
    reset();
    cap_begin("RAM:", "empty");
    CHECK(cap_end() == 0);
    CHECK(files["RAM:empty.txt"] == "# capture empty 1\n");
    cap_begin("", "bare");
    CHECK(cap_end() == 0);
    CHECK(files["bare.txt"] == "# capture bare 1\n");
}

TEST_CASE("the occurrence numbers start again with each program", "[capture]")
{
    reset();
    for (int i = 0; i < 2; i++) {
        cap_begin("TEST:", "again");
        cap_routine("r");
        cap_num("c", "v", 1);
        CHECK(cap_end() == 0);
        CHECK(files["TEST:again.txt"] == "# capture again 1\nr|c|v|1\t$00000001\n");
    }
}

TEST_CASE("block cases", "[capture]")
{
    reset();
    have_dumpvar = true;
    dumpvar = "BLOCK:2";
    cap_begin("TEST:", "blk");
    cap_routine("med");
    cap_num("before", nullptr, 1);
    cap_blocks(1);
    std::string lines1, lines2;
    for (uint32_t i = 1; i <= CAP_BLOCK + 3; i++) {
        cap_num("obs", "v", i);
        std::string l = "med|obs|v|" + std::to_string(i) + "\t$" + hex8(i) + "\n";
        (i <= CAP_BLOCK ? lines1 : lines2) += l;
        if (i == CAP_BLOCK + 1) {
            cap_not_captured("skip", nullptr, "why");
        }
    }
    cap_blocks(0);
    cap_num("after", nullptr, 2);
    CHECK(cap_end() == 0);
    CHECK(failures.empty());

    const std::string want = std::string("# capture blk 1\n")
        + "med|before||1\t$00000001\n"
        + "BLOCK:1\t4096 " + hex8(fnv(lines1)) + " first=med|obs|v|1\n"
        + "med|skip||1\tnot captured: why\n"
        + "BLOCK:2\t3 " + hex8(fnv(lines2)) + " first=med|obs|v|4097\n"
        + "med|after||1\t$00000002\n";
    CHECK(files["TEST:blk.txt"] == want);
    // the dump of block 2: its observation lines, whose hash is the block's
    CHECK(files["TEST:blk.dump"] == lines2);
}

TEST_CASE("LIBS_CAP_DUMP of a long buffer", "[capture]")
{
    reset();
    unsigned char b[70];
    for (int i = 0; i < 70; i++)
        b[i] = (unsigned char)(0xA0 + i % 16);
    have_dumpvar = true;
    dumpvar = "r|buf|v|2";
    cap_begin("TEST:", "dmp");
    cap_routine("r");
    cap_bytes("buf", "v", b, 70);
    cap_bytes("buf", "v", b, 70);
    CHECK(cap_end() == 0);
    std::string row = "A0A1A2A3A4A5A6A7A8A9AAABACADAEAF";
    CHECK(files["TEST:dmp.dump"] == "r|buf|v|2\tat $0000 " + row + row + "\n"
                                        + "r|buf|v|2\tat $0020 " + row + row + "\n"
                                        + "r|buf|v|2\tat $0040 A0A1A2A3A4A5\n");
    CHECK(files.count("TEST:dmp.txt") == 1);
}

TEST_CASE("broken: the output cannot be opened", "[capture]")
{
    reset();
    refuse_create = true;
    cap_begin("TEST:nodir", "fail");
    cap_num("c", nullptr, 1);
    CHECK(cap_end() == 1);
    REQUIRE(failures.size() == 1);
    CHECK(failures[0] == "capture fail: cannot open the output");
    CHECK(dirs == std::vector<std::string>{"TEST:nodir"});
}

TEST_CASE("broken: a key too long", "[capture]")
{
    reset();
    std::string longcheck(2000, 'k');
    cap_begin("TEST:", "long");
    cap_num(longcheck.c_str(), nullptr, 1);
    CHECK(cap_end() == 1);
    REQUIRE(failures.size() == 1);
    CHECK(failures[0] == "capture long: a key is too long");
}

TEST_CASE("broken: too many distinct keys", "[capture]")
{
    reset();
    cap_begin("TEST:", "many");
    char v[16];
    for (uint32_t i = 0; i < 120000; i++) {
        std::snprintf(v, sizeof v, "%lu", (unsigned long)i);
        cap_num("c", v, i);
    }
    CHECK(cap_end() == 1);
    REQUIRE(failures.size() == 1);
    CHECK(failures[0] == "capture many: too many distinct keys");
}

TEST_CASE("without cap_begin nothing is written", "[capture]")
{
    reset();
    cap_num("c", nullptr, 1);
    CHECK(cap_end() == 0);
    CHECK(files.empty());
}
