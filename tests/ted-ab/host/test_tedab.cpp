// test_tedab - the TED A/B runner without emulator (runner/tedab.h):
// directives, lint, option block, requester sequence, session script,
// session log, oracle lookup.  From bb2 ted-c/tests/tools/test_tedab.py.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <sys/stat.h>
#include <unistd.h>
#include <snitch/snitch.hpp>
extern "C" {
#include "tedab.h"
}

namespace {

std::string lint(const char *text, int *n = nullptr)
{
    char *buf = nullptr;
    size_t len = 0;
    FILE *m = open_memstream(&buf, &len);
    int k = tedab_lint(text, m);

    fclose(m);
    std::string s(buf ? buf : "");
    free(buf);
    if (n)
        *n = k;
    return s;
}

struct Test {
    tedab_test t{};
    int rc;
    char err[300] = "";

    Test(const char *text, const char *name = "x") { rc = tedab_test_parse(name, text, &t, err, sizeof err); }
    ~Test() { tedab_test_free(&t); }
};

std::vector<std::string> requesters(const char *log)
{
    size_t n;
    char **l = tedab_requesters(log, &n);
    std::vector<std::string> v;

    for (size_t i = 0; i < n; i++)
        v.emplace_back(l[i]);
    tedab_free_lines(l);
    return v;
}

}  // namespace

TEST_CASE("directives", "[test]")
{
    Test a("; @source T t01_int\n; @roundtrip\n; @expect absent t01_int.bb2.xtra\n"
           "; @xtra norunerr\n; @est 4\n; @combos R5 C5\ntimeout 10\n");
    REQUIRE(a.rc == 0);
    CHECK(a.t.src_kind == 'T');
    CHECK(std::string(a.t.file) == "t01_int.bb2");
    CHECK(a.t.norunerr == 1);
    CHECK(a.t.est == 4);
    CHECK(a.t.combos == ((1u << TEDAB_R5) | (1u << TEDAB_C5)));
    // @roundtrip's expectations first
    REQUIRE(a.t.nexpects == 3);
    CHECK(std::string(a.t.expects[0].kind) == "same");
    CHECK(std::string(a.t.expects[1].kind) == "bak");
    CHECK(std::string(a.t.expects[1].a) == "t01_int.bb2.bak");
    CHECK(std::string(a.t.expects[2].kind) == "absent");

    Test k("; @source K internet/demos/memo.bb2\n; @expect tedsave memo.bb2 internet/demos/memo.bb2\n");
    REQUIRE(k.rc == 0);
    CHECK(std::string(k.t.file) == "memo.bb2");
    CHECK(k.t.combos == 15u);

    Test none("; @source none\n; @nofiles\n");
    REQUIRE(none.rc == 0);
    CHECK(none.t.file == nullptr);

    // a comment that mentions an @word in text is no directive
    Test text("; @source T t01_int\n; original, @roundtrip), .bak\n; @nofiles\n");
    REQUIRE(text.rc == 0);
    CHECK(text.t.roundtrip == 0);
    CHECK(text.t.known == nullptr);

    // @known keeps the rest of the line as its reason
    Test kn("; @source none\n; @nofiles\n; @known KNOWN-BOTH (28.10.2026): 513 rows\r\n");
    REQUIRE(kn.rc == 0);
    CHECK(std::string(kn.t.known) == "KNOWN-BOTH (28.10.2026): 513 rows");
}

TEST_CASE("directives refused", "[test]")
{
    CHECK(Test("; @roundtrip\n").rc == -1);                                // no @source
    CHECK(Test("; @source T t01_int\n").rc == -1);                         // OK alone is no pass
    CHECK(Test("; @source T t01_int\n; @expect same\n").rc == -1);         // arity
    CHECK(Test("; @source T t01_int\n; @expect equals a b\n").rc == -1);   // bb2's kind, gone
    CHECK(Test("; @source T t01_int\n; @nofiles\n; @combos R5 X9\n").rc == -1);
    CHECK(Test("; @source Q x\n; @nofiles\n").rc == -1);
    CHECK(Test("; @source none\n; @roundtrip\n").rc == -1);                // nothing to compare
    CHECK(Test("; @source T a\n; @nofiles\n; @opts nosuch=1\n").rc == -1);
    CHECK(Test("; @source none\n; @nofiles\n; @known\n").rc == -1);    // no reason
    Test t("; @source T a\n; @nofiles\n; @font 7\n");
    CHECK(t.rc == -1);
    CHECK(std::string(t.err).find("@font") != std::string::npos);
}

TEST_CASE("lint", "[lint]")
{
    int n;

    CHECK(lint("; comment\ntimeout 10\nwait req \"OKEE DOKEE\"\ngad 1\nkey $45 1\n"
               "menu 0 2\nfreq ok \"TEDRUN:work/a.bb2\"\nexpectfail wait exit\nlog\n", &n) == "");
    CHECK(n == 0);
    CHECK(lint("frob 1\n", &n) == "line 1: unknown command frob\n");
    CHECK(lint("wait\n", &n) == "line 1: unknown wait (none)\n");
    CHECK(lint("wait screen 1\n", &n) == "line 1: wait screen: 1 arguments\n");
    CHECK(lint("menu 1\n", &n) == "line 1: menu: 1 arguments\n");
    CHECK(lint("freq maybe\n", &n) == "line 1: freq ok|cancel\n");
    CHECK(lint("freq ok \"a.bb2\"\n", &n).find("relative name without PATH") != std::string::npos);
    CHECK(lint("freq ok \"a\" \"TEDRUN:work\"\n", &n) == "");
    CHECK(lint("gad x\n", &n) == "line 1: gad: bad number 'x'\n");
    CHECK(lint("key 4G\n", &n) == "line 1: key: bad number '4G'\n");
    CHECK(lint("key 45\nmenu $1 2\n", &n) == "");     // key: hex, "$" = hex
    CHECK(lint("expectfail\n", &n) == "line 1: expectfail without command\n");
    std::string longline = "type \"" + std::string(290, 'a') + "\"\n";
    CHECK(lint(longline.c_str(), &n).find("longer than 290") != std::string::npos);
}

TEST_CASE("option block", "[opts]")
{
    uint8_t o[50];
    char err[200];

    REQUIRE(tedab_opts_file("", o, err, sizeof err) == 0);
    // fonthite 8, fonthite2 7, fontmul 1280 ...
    CHECK(o[0] == 0);
    CHECK(o[1] == 8);
    CHECK(o[3] == 7);
    CHECK(o[4] == 0x05);
    CHECK(o[5] == 0x00);
    CHECK(o[23] == 2);              // tabstop
    CHECK(o[24 * 2 + 1] == 0);      // the last word
    CHECK(o[12 * 2 + 1] == 8);      // palette
    REQUIRE(tedab_opts_file("fonthite=6,fonthite2=5,fontmul=960", o, err, sizeof err) == 0);
    CHECK(o[1] == 6);
    CHECK(o[3] == 5);
    CHECK(((o[4] << 8) | o[5]) == 960);
    CHECK(tedab_opts_file("tabstop=x", o, err, sizeof err) == -1);
    CHECK(tedab_opts_file("tab=1", o, err, sizeof err) == -1);
    CHECK(std::string(err) == "unknown option field tab");
}

TEST_CASE("requesters", "[log]")
{
    const char *log =
        "step 1 wait req \"OKEE DOKEE\"\n"
        "window new #1 main \"\" pos 0,12 size 640,497 flags $0 gadgets 2\n"
        "  gadget id 1 type $00000001 pos 1,2 size 104,12 \"Main\"\n"
        "window new #2 requester \"Blitz Basic 2\" pos 10,20 size 300,80 flags $0 gadgets 2\n"
        "  gadget id 1 type $00000001 flags $0 size 104,12 \" OKEE DOKEE \"\n"
        "  gadget id 2 type $00000004 flags $0 size -10,12 \"name\" buffer \"x\"\n"
        "step 2 gad 1\n"
        "window new #3 requester \"a \" pos x\" pos 1,1 size 1,1 gadgets 0\n";
    auto v = requesters(log);

    REQUIRE(v.size() == 2);
    CHECK(v[0] == "requester Blitz Basic 2 1:OKEE DOKEE 2:name");
    CHECK(v[1] == "requester a \" pos x");
    CHECK(requesters("").empty());
}

TEST_CASE("session script", "[script]")
{
    Test a("; @source T t01_int\n; @roundtrip\n", "01a-rt-t01");
    Test b("; @source none\n; @nofiles\n", "00-start");
    REQUIRE(a.rc == 0);
    REQUIRE(b.rc == 0);
    tedab_run runs[3] = { { &a.t, TEDAB_R5 }, { &a.t, TEDAB_C5 }, { &b.t, TEDAB_R5 } };
    char *s = tedab_session_script(runs, 3, 40000);

    REQUIRE(s != nullptr);
    std::string sc(s);
    CHECK(sc.find("If EXISTS TEDAB:booted\n") == 0);
    CHECK(sc.find("  Stack 40000\n") != std::string::npos);
    CHECK(sc.find("  TEDAB:tools/tedprobe TEDAB:gate/out/gate0.txt d >NIL:\n") != std::string::npos);
    CHECK(sc.find("TEDAB:tools/reqtrap TEDAB:tools/teddrive TEDAB:tests/01a-rt-t01.drv TEDRUN:out/ "
                  "blitz2:blitz2 TEDRUN:work/t01_int.bb2 >TEDRUN:out/stdout\n") != std::string::npos);
    CHECK(sc.find("TEDAB:tests/00-start.drv TEDRUN:out/ blitz2:blitz2 >TEDRUN:out/stdout\n")
          != std::string::npos);
    // the other combination needs the test's first one, the next test the
    // session's first run
    CHECK(sc.find("If EXISTS TEDAB:runs/01a-rt-t01/R5/out/ok\n") != std::string::npos);
    CHECK(sc.find("skipped (01a-rt-t01 R5 not OK)") != std::string::npos);
    CHECK(sc.find("Assign blitz2: TEDAB:combo/C5/blitz2\n") != std::string::npos);
    CHECK(sc.find("TEDAB:gate/out/gr. TEDAB:tools/teddrivetest rescue >TEDAB:gate/out/gr.stdout")
          != std::string::npos);

    char *buf = nullptr;
    size_t len = 0;
    FILE *m = open_memstream(&buf, &len);
    CHECK(tedab_check_script(s, m) == 0);
    fclose(m);
    free(buf);
    free(s);
}

TEST_CASE("session waits for", "[script]")
{
    // as session 011 of a run that hit #340: 28-long R5 first, then the
    // other combinations, then the next test
    Test a("; @source none\n; @nofiles\n", "28-long");
    Test b("; @source none\n; @nofiles\n", "29-cmp-t02_float");
    tedab_run runs[5] = { { &a.t, TEDAB_R5 }, { &a.t, TEDAB_C5 }, { &a.t, TEDAB_CC },
                          { &b.t, TEDAB_R5 }, { &b.t, TEDAB_C5 } };

    CHECK(tedab_session_waits_for(runs, 0) == 0);
    CHECK(tedab_session_waits_for(runs, 1) == 0);
    CHECK(tedab_session_waits_for(runs, 2) == 0);
    CHECK(tedab_session_waits_for(runs, 3) == 0);   // a new test: the session's first
    CHECK(tedab_session_waits_for(runs, 4) == 3);   // its reference

    // the script says the same
    char *s = tedab_session_script(runs, 5, 40000);
    REQUIRE(s != nullptr);
    std::string sc(s);
    CHECK(sc.find("RUN 29-cmp-t02_float R5 skipped (28-long R5 not OK)") != std::string::npos);
    CHECK(sc.find("RUN 29-cmp-t02_float C5 skipped (29-cmp-t02_float R5 not OK)")
          != std::string::npos);
    free(s);
}

TEST_CASE("script check", "[script]")
{
    char *buf = nullptr;
    size_t len = 0;
    FILE *m = open_memstream(&buf, &len);
    std::string lng = "Echo " + std::string(260, 'x') + "\n";

    CHECK(tedab_check_script("If A\n  If B\n  EndIf\n", m) == 1);
    CHECK(tedab_check_script("EndIf\n", m) == 1);
    CHECK(tedab_check_script(lng.c_str(), m) == 1);
    CHECK(tedab_check_script("IfX\nEndIfY\n", m) == 0);    // other words
    fclose(m);
    CHECK(std::string(buf).find("script: 1 If without EndIf") != std::string::npos);
    CHECK(std::string(buf).find("EndIf without If") != std::string::npos);
    free(buf);
}

TEST_CASE("session log", "[log]")
{
    Test a("; @source none\n; @nofiles\n", "00-start");
    Test b("; @source none\n; @nofiles\n", "02-type");
    tedab_run runs[4] = { { &a.t, TEDAB_R5 }, { &a.t, TEDAB_C5 }, { &b.t, TEDAB_R5 },
                          { &b.t, TEDAB_C5 } };
    int st[4];
    text ev{};

    CHECK(tedab_session_log("SESSION start\nGATE0 OK\nGATED OK\nRUN 00-start R5 start\n"
                            "RUN 00-start R5 end\nRUN 00-start C5 start\n"
                            "RUN 02-type R5 skipped (00-start R5 not OK)\n", runs, 4, st, &ev) == 4);
    CHECK(st[0] == TEDAB_ENDED);
    CHECK(st[1] == TEDAB_STARTED);
    CHECK(st[2] == TEDAB_SKIPPED);
    CHECK(st[3] == TEDAB_NOTRUN);
    CHECK(std::string(ev.p) == "SESSION start, GATE0 OK, GATED OK");
    free(ev.p);
}

TEST_CASE("oracle", "[files]")
{
    char dir[] = "/tmp/test_tedab_XXXXXX";
    REQUIRE(mkdtemp(dir) != nullptr);
    std::string p = std::string(dir) + "/m.sha256";
    std::string h(64, 'a');
    FILE *f = fopen(p.c_str(), "w");

    REQUIRE(f != nullptr);
    fprintf(f, "%s  detok/INTERNET/MEMO.BB2\n%s  save/INTERNET/MEMO.BB2X\n", std::string(64, 'b').c_str(),
            std::string(64, 'c').c_str());
    fprintf(f, "%s  save/INTERNET/MEMO.BB2\n", h.c_str());
    fclose(f);
    char hex[65];
    REQUIRE(tedab_oracle_hash(p.c_str(), "internet/memo.bb2", hex) == 0);
    CHECK(std::string(hex) == h);
    CHECK(tedab_oracle_hash(p.c_str(), "internet/memo", hex) == -1);
    // resolve without case
    std::string sub = std::string(dir) + "/SUB";
    mkdir(sub.c_str(), 0755);
    char *r = tedab_resolve(dir, "sub");
    REQUIRE(r != nullptr);
    CHECK(std::string(r) == sub);
    free(r);
    CHECK(tedab_resolve(dir, "sub/m.sha256") == nullptr);
    r = tedab_resolve(dir, "M.SHA256");
    REQUIRE(r != nullptr);
    CHECK(std::string(r) == p);
    free(r);
    CHECK(tedab_resolve(dir, "nosuch") == nullptr);
    remove(p.c_str());
    rmdir(sub.c_str());
    rmdir(dir);
}

TEST_CASE("postfix", "[postfix]")
{
    auto pf = [](const char *s) { return tedab_postfix(s, strlen(s)); };

    CHECK(pf("freq ok \"a\" \"TEDRUN:work\"") == 0);
    CHECK(pf("freq ok \"a\" \"TEDRUN:work\"  ; @ignore compilewindow") == 1);
    CHECK(pf("freq ok \"a\" \"TEDRUN:work\" ;@ignore   compilewindow \r") == 1);
    CHECK(pf("gad 1 ; a plain comment") == 0);
    CHECK(pf("gad 1 ; @ignore everything") == -1);
    CHECK(pf("gad 1 ; @unless compilewindow") == -1);
    CHECK(pf("type \"x ; @ignore compilewindow\"") == 0);      // inside the string
    CHECK(pf("type \"a\\\"b ; @frob\"") == 0);                   // escaped quote
    CHECK(pf("key 45;@ignore compilewindow") == 0);             // not at a word start

    int n;
    CHECK(lint("freq ok \"a\" \"TEDRUN:work\"  ; @ignore compilewindow\n", &n) == "");
    CHECK(lint("gad 1 ; @frob\n", &n) == "line 1: unknown postfix (only ; @ignore compilewindow)\n");
    CHECK(lint("; @frob directive lines are no commands\n", &n) == "");
}

TEST_CASE("requesters: compile window", "[log]")
{
    const char *log =
        "step 7 t=0.90: timeout 30\n"
        "  ok t=0.90\n"
        "step 8 t=0.90: freq ok \"t01_int\" \"TEDRUN:work\"  ; @ignore compilewindow\n"
        "window closed #3 \"Name of Executable to create\"\n"
        "window new #4 requester \"\" pos 202,225 size 220,59 flags $0 gadgets 1\n"
        "  gadget id 1 type $00000001 flags $0 size 60,12 \"STOP!\"\n"
        "window closed #4 \"\"\n"
        "window new #5 requester \"\" pos 202,225 size 220,59 flags $0 gadgets 1\n"
        "  gadget id 1 type $00000001 flags $0 size 60,12 \"OKAY\"\n"
        "step 9 t=1.18: timeout 10\n"
        "window new #6 requester \"\" pos 202,225 size 220,59 flags $0 gadgets 1\n"
        "  gadget id 1 type $00000001 flags $0 size 60,12 \"STOP!\"\n";
    auto v = requesters(log);

    // the compile window counts outside the marked step, other requesters always
    REQUIRE(v.size() == 2);
    CHECK(v[0] == "requester  1:OKAY");
    CHECK(v[1] == "requester  1:STOP!");
}

TEST_CASE("bug-orig directives", "[bugorig]")
{
    // @file with escapes (also \x00), repeated lines append; @source F,
    // @command as it is; the quoted @expect kinds
    Test a("; @file bo.bb2 \"x\\x00\\xdd\\x82\\ta\"\n; @file bo.bb2 \"\\x00\"\n"
           "; @source F bo.bb2\n; @command TEDAB:tools/inject track blitz2:ted @FILE@\n"
           "; @expect data bo.bb2 \"x\\x00\"\n; @expect data bo.bb2 \"\\\"q\\\" b\"\n"
           "; @expect stdout \"inject: rc=0\"\n; @expect noscreen joined \"\\xc2\\xb7loop\"\n"
           "; @expect nolog \"window new\"\ntimeout 10\n");
    REQUIRE(a.rc == 0);
    CHECK(a.t.src_kind == 'F');
    CHECK(std::string(a.t.file) == "bo.bb2");
    CHECK(std::string(a.t.command) == "TEDAB:tools/inject track blitz2:ted @FILE@");
    REQUIRE(a.t.nfiles == 1);
    CHECK(std::string((const char *)a.t.files[0].data, a.t.files[0].len)
          == std::string("x\0\xdd\x82\ta\0", 7));
    REQUIRE(a.t.nexpects == 4);
    CHECK(std::string(a.t.expects[0].kind) == "data");
    CHECK(std::string((const char *)a.t.expects[0].data, a.t.expects[0].len)
          == std::string("x\0\"q\" b", 7));
    CHECK(std::string(a.t.expects[1].kind) == "stdout");
    CHECK(std::string((const char *)a.t.expects[1].data) == "inject: rc=0");
    CHECK(std::string(a.t.expects[2].a) == "joined");
    CHECK(std::string((const char *)a.t.expects[2].data) == "\xc2\xb7loop");
    CHECK(std::string(a.t.expects[3].kind) == "nolog");

    // refused: F without its @file, a kind without its string, text behind
    // the string, an unterminated string, @file without a name, @command
    // without a command
    CHECK(Test("; @source F x.bas\n; @nofiles\n").rc == -1);
    CHECK(Test("; @source none\n; @expect stdout rc=0\n").rc == -1);
    CHECK(Test("; @source none\n; @expect stdout \"rc=0\" x\n").rc == -1);
    CHECK(Test("; @source none\n; @expect screen d \"abc\n").rc == -1);
    CHECK(Test("; @source none\n; @nofiles\n; @file \"abc\"\n").rc == -1);
    CHECK(Test("; @source none\n; @nofiles\n; @command\n").rc == -1);
}

TEST_CASE("bug-orig: @command in the session script", "[bugorig]")
{
    Test a("; @file bo.bas \"a\\n\"\n; @source F bo.bas\n; @command blitz2:ted @FILE@\n"
           "; @nofiles\n", "ted-x");
    REQUIRE(a.rc == 0);
    tedab_run runs[1] = { { &a.t, TEDAB_R5 } };
    char *s = tedab_session_script(runs, 1, 40000);

    REQUIRE(s != nullptr);
    CHECK(std::string(s).find("TEDAB:tests/ted-x.drv TEDRUN:out/ blitz2:ted TEDRUN:work/bo.bas "
                              ">TEDRUN:out/stdout\n") != std::string::npos);
    free(s);
}

TEST_CASE("bug-orig expectations", "[bugorig]")
{
    char dir[] = "/tmp/test_tedab_XXXXXX";
    REQUIRE(mkdtemp(dir) != nullptr);
    std::string d(dir);
    mkdir((d + "/work").c_str(), 0755);
    mkdir((d + "/out").c_str(), 0755);
    auto put = [&](const std::string &name, const std::string &data) {
        FILE *f = fopen((d + "/" + name).c_str(), "wb");
        fwrite(data.data(), 1, data.size(), f);
        fclose(f);
    };
    put("work/blk.txt", std::string("a\0b", 3));
    put("out/stdout", "inject: rc=0\nrc=0\n");
    put("out/log", "step 1\n");
    put("out/joined.txt", "  0 |a=1.loop  \xc2\xb7\xc2\xb7loop\n");
    tedab_sources src = { "R5", nullptr, nullptr, nullptr, nullptr, nullptr };
    auto check = [&](const char *script, std::string *rep = nullptr) {
        Test t(script);
        REQUIRE(t.rc == 0);
        text out = {};
        int n = tedab_check_expects(&t.t, dir, &src, &out);
        if (rep)
            *rep = out.p ? out.p : "";
        free(out.p);
        return n;
    };
    std::string rep;

    CHECK(check("; @source none\n; @expect data blk.txt \"a\\x00b\"\n") == 0);
    CHECK(check("; @source none\n; @expect data blk.txt \"a\\x00c\"\n", &rep) == 1);
    CHECK(rep == "  R5 expect: blk.txt differs from the expected contents (data)\n");
    CHECK(check("; @source none\n; @expect data nosuch \"a\"\n") == 1);
    CHECK(check("; @source none\n; @expect stdout \"inject: rc=0\"\n") == 0);
    CHECK(check("; @source none\n; @expect nostdout \"inject: rc=0\"\n", &rep) == 1);
    CHECK(rep == "  R5 expect: stdout has \"inject: rc=0\"\n");
    // the text in the report as in the directive
    CHECK(check("; @source none\n; @expect nostdout \"rc=0\\n\"\n", &rep) == 1);
    CHECK(rep == "  R5 expect: stdout has \"rc=0\\n\"\n");
    CHECK(check("; @source none\n; @expect log \"step 1\"\n; @expect nolog \"step 2\"\n") == 0);
    CHECK(check("; @source none\n; @expect screen joined \"a=1.loop\"\n") == 0);
    CHECK(check("; @source none\n; @expect noscreen joined \"\\xc2\\xb7loop\"\n", &rep) == 1);
    CHECK(rep == "  R5 expect: screen joined has \"\xc2\xb7loop\"\n");
    CHECK(check("; @source none\n; @expect screen nosuch \"x\"\n", &rep) == 1);
    CHECK(rep == "  R5 expect: dump nosuch.txt missing\n");
    CHECK(check("; @source none\n; @expect file blk.txt \"a\"\n") == 0);
    CHECK(check("; @source none\n; @expect nofile blk.txt \"a\"\n", &rep) == 1);
    CHECK(rep == "  R5 expect: file blk.txt has \"a\"\n");
    CHECK(check("; @source none\n; @expect file nosuch \"a\"\n", &rep) == 1);
    CHECK(rep == "  R5 expect: nosuch missing\n");
    // a missing file has no text in it
    CHECK(check("; @source none\n; @expect noscreen nosuch \"x\"\n") == 0);
    for (const char *f : { "work/blk.txt", "out/stdout", "out/log", "out/joined.txt" })
        remove((d + "/" + f).c_str());
    rmdir((d + "/work").c_str());
    rmdir((d + "/out").c_str());
    rmdir(dir);
}

TEST_CASE("bug-orig verdict", "[bugorig]")
{
    const char *bad = "  - expect: blk.txt differs from the expected contents (data)\n";

    // the reference shows the correct behaviour: the test is broken
    CHECK(tedab_bugorig_verdict("OK", "", "OK", bad, 0) == TEDAB_BROKEN);
    CHECK(tedab_bugorig_verdict("OK", "", "OK", "", 0) == TEDAB_BROKEN);
    // the port is correct
    CHECK(tedab_bugorig_verdict("OK", bad, "OK", "", 1) == TEDAB_FIXED);
    CHECK(tedab_bugorig_verdict("TIMEOUT", "", "OK", "", -1) == TEDAB_FIXED);
    // the port shows what the original shows
    CHECK(tedab_bugorig_verdict("OK", bad, "OK", bad, 0) == TEDAB_REPRODUCED);
    CHECK(tedab_bugorig_verdict("TIMEOUT", "", "TIMEOUT", "", -1) == TEDAB_REPRODUCED);
    CHECK(tedab_bugorig_verdict("FAIL step 3 timeout", bad, "FAIL step 3 timeout", bad, -1)
          == TEDAB_REPRODUCED);
    // neither
    CHECK(tedab_bugorig_verdict("OK", bad, "OK", bad, 1) == TEDAB_DIFFERS);
    CHECK(tedab_bugorig_verdict("TIMEOUT", "", "FAIL step 3 timeout", "", -1) == TEDAB_DIFFERS);
    CHECK(tedab_bugorig_verdict("OK", bad, "OK", "  - expect: x missing\n", 0) == TEDAB_DIFFERS);
    CHECK(std::string(tedab_verdict_name(TEDAB_REPRODUCED)) == "REPRODUCED");
    CHECK(std::string(tedab_verdict_name(TEDAB_BROKEN)) == "BROKEN");
}
