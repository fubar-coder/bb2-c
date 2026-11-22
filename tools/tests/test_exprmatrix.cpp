// test_exprmatrix - the expression matrix (lib/exprmatrix): the description
// read into keyed cells (the product of a block, cell lines, the cells BASIC
// has no meaning for left out), the broken descriptions (unknown kind,
// operator, context and value set, a duplicate key, bad values, missing
// values), and the generated sources (the output forms of design decision
// 8 of the OpenSpec change expression-type-matrix, the split at N cells).
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <snitch/snitch.hpp>
extern "C" {
#include "exprmatrix.h"
}

static const char *const SETS =
    "values A b=3 w=1000 l=100000 q=1.5 f=2.5 s=AB cb=3 cw=1000 cl=100000 cf=1.5\n"
    "counts A b=2 w=2 l=2 q=2 f=2 cb=2 cw=2 cl=2 cf=2.5\n"
    "values B b=-3 w=-1000 l=-100000 q=-1.5 f=-2.5 cb=-3 cw=-1000 cl=-100000 cf=-1.5\n"
    "counts B b=3 w=3 l=3 q=3 f=3 cb=3 cw=3 cl=3 cf=3\n";

// the description parsed; ok: it must parse (else: the errors, which must not be empty)
static std::string parse(const std::string &desc, struct em_matrix &m, bool ok = true)
{
    char *errors = nullptr;
    int r = em_parse(desc.data(), desc.size(), "m.txt", &m, &errors);
    std::string e = errors ? errors : "";

    std::free(errors);
    if (ok && r != 0)
        std::fprintf(stderr, "em_parse: %d\n%s", r, e.c_str());
    if (!ok && r != 1)
        std::fprintf(stderr, "em_parse: %d, wanted 1\n", r);
    return ok ? (r == 0 ? "" : "failed") : (r == 1 ? e : "");
}

// the description is refused with a message containing msg
static bool refused(const std::string &desc, const char *msg)
{
    struct em_matrix m;
    std::string e = parse(desc, m, false);

    if (e.find(msg) == std::string::npos) {
        std::fprintf(stderr, "wanted \"%s\", got \"%s\"\n", msg, e.c_str());
        return false;
    }
    return m.ncells == 0;
}

static std::vector<std::string> keys(const struct em_matrix &m)
{
    std::vector<std::string> v;

    for (size_t i = 0; i < m.ncells; i++)
        v.push_back(m.cells[i].key);
    return v;
}

TEST_CASE("exprmatrix: names", "[exprmatrix]")
{
    CHECK(std::string(em_kind_name(EM_CF)) == "cf");
    CHECK(std::string(em_op_name(EM_ASR)) == "asr");
    CHECK(std::string(em_op_text(EM_ASR)) == "ASR");
    CHECK(std::string(em_op_text(EM_NE)) == "<>");
    CHECK(std::string(em_ctx_name(EM_PRINT)) == "print");
    CHECK(em_op_is_unary(EM_NOT));
    CHECK(!em_op_is_unary(EM_SUB));
    CHECK(em_op_is_compare(EM_GE));
    CHECK(!em_op_is_compare(EM_ASR));
}

TEST_CASE("exprmatrix: the product of a block", "[exprmatrix]")
{
    struct em_matrix m;
    std::string d = std::string(SETS) +
                    "# a comment\n"
                    "block arith   # an operator group\n"
                    "contexts dl print\n"
                    "binary + mod\n"
                    "unary - not\n"
                    "kinds q l\n"
                    "sets A\n";

    REQUIRE(parse(d, m) == "");
    std::vector<std::string> want = {
        "dl-neg-q-A", "dl-neg-l-A", "dl-not-q-A", "dl-not-l-A",
        "dl-add-q-q-A", "dl-add-q-l-A", "dl-add-l-q-A", "dl-add-l-l-A",
        "dl-mod-q-q-A", "dl-mod-q-l-A", "dl-mod-l-q-A", "dl-mod-l-l-A",
        "print-neg-q-A", "print-neg-l-A", "print-not-q-A", "print-not-l-A",
        "print-add-q-q-A", "print-add-q-l-A", "print-add-l-q-A", "print-add-l-l-A",
        "print-mod-q-q-A", "print-mod-q-l-A", "print-mod-l-q-A", "print-mod-l-l-A",
    };
    CHECK(keys(m) == want);
    CHECK(m.per_source == EM_PER_SOURCE_DEFAULT);
    CHECK(m.dim == EM_DIM_DEFAULT);
    REQUIRE(m.nblocks == 1);
    CHECK(std::string(m.blocks[0].name) == "arith");
    REQUIRE(m.nsets == 2);
    CHECK(std::string(m.sets[1].value[EM_CF]) == "-1.5");
    em_free(&m);
}

TEST_CASE("exprmatrix: both orders, every kind and the strings", "[exprmatrix]")
{
    struct em_matrix m;
    std::string d = std::string(SETS) +
                    "block all\n"
                    "contexts db dw dl dq df pl pw pq pf if i1 i2 print\n"
                    "binary + - * / MOD ^ AND OR EOR LSL LSR ASL ASR = <> < <= > >=\n"
                    "unary - NOT\n"
                    "kinds b w l q f s cb cw cl cf\n"
                    "sets A\n";

    REQUIRE(parse(d, m) == "");
    // 13 contexts x (19 operators x 9 x 9 numeric pairs + 2 x 9 unary + 6 string comparisons)
    // + the string + in print
    CHECK(m.ncells == 13 * (19 * 81 + 18 + 6) + 1);
    std::vector<std::string> k = keys(m);
    auto has = [&](const char *key) { return std::find(k.begin(), k.end(), key) != k.end(); };
    CHECK(has("pw-add-q-w-A"));
    CHECK(has("pw-add-w-q-A"));
    CHECK(has("print-add-s-s-A"));
    CHECK(!has("dl-add-s-s-A"));
    CHECK(has("dl-lt-s-s-A"));
    CHECK(!has("dl-mul-s-s-A"));
    CHECK(!has("dl-eq-s-l-A"));
    CHECK(!has("dl-neg-s-A"));
    em_free(&m);
}

TEST_CASE("exprmatrix: cell lines, two sets, the settings", "[exprmatrix]")
{
    struct em_matrix m;
    std::string d = std::string(SETS) +
                    "cells-per-source 7\n"
                    "dim 50\n"
                    "block one\n"
                    "cell A dl * q l\n"
                    "cell B i2 - cf\n"
                    "cell A print + s s\n"
                    "block two\n"
                    "contexts if\n"
                    "binary asr\n"
                    "kinds w\n"
                    "sets A B\n";

    REQUIRE(parse(d, m) == "");
    std::vector<std::string> want = { "dl-mul-q-l-A", "i2-neg-cf-B", "print-add-s-s-A", "if-asr-w-w-A",
                                      "if-asr-w-w-B" };
    CHECK(keys(m) == want);
    CHECK(m.per_source == 7);
    CHECK(m.dim == 50);
    CHECK(m.cells[0].line == 8);
    CHECK(m.cells[0].block == 0);
    CHECK(m.cells[3].block == 1);
    CHECK(m.cells[4].set == 1);
    em_free(&m);
}

TEST_CASE("exprmatrix: broken descriptions", "[exprmatrix]")
{
    std::string s = SETS;

    // unknown names
    CHECK(refused(s + "block x\ncontexts dl\nbinary +\nkinds q z\nsets A\n", "m.txt:8: unknown kind \"z\""));
    CHECK(refused(s + "block x\ncontexts dl\nbinary + %\nkinds q\nsets A\n",
                  "m.txt:7: unknown operator \"%\""));
    CHECK(refused(s + "block x\ncontexts dl\nunary NOT +\nkinds q\nsets A\n", "unknown operator \"+\""));
    CHECK(refused(s + "block x\ncontexts dl dz\nbinary +\nkinds q\nsets A\n",
                  "m.txt:6: unknown context \"dz\""));
    CHECK(refused(s + "block x\ncontexts dl\nbinary +\nkinds q\nsets C\n", "unknown value set \"C\""));
    CHECK(refused(s + "block x\ncell A dl + q x\n", "unknown kind \"x\""));
    CHECK(refused(s + "block x\ncell A dx + q l\n", "unknown context \"dx\""));
    CHECK(refused(s + "block x\ncell A dl ** q l\n", "unknown operator \"**\""));
    CHECK(refused(s + "block x\ncell Z dl + q l\n", "unknown value set \"Z\""));
    CHECK(refused(s + "values A z=1\n", "unknown kind \"z\""));
    CHECK(refused(s + "frobnicate 3\n", "m.txt:5: unknown directive \"frobnicate\""));

    // duplicate keys: one cell twice, two blocks making the same cell
    CHECK(refused(s + "block x\ncell A dl + q l\ncell A dl + q l\n",
                  "m.txt:7: duplicate key dl-add-q-l-A (also line 6)"));
    CHECK(refused(s + "block x\ncontexts dl\nbinary +\nkinds q l\nsets A\nblock y\ncell A dl + l q\n",
                  "duplicate key dl-add-l-q-A"));

    // given twice
    CHECK(refused(s + "block x\ncontexts dl dl\nbinary +\nkinds q\nsets A\n", "context dl given twice"));
    CHECK(refused(s + "block x\ncontexts dl\nbinary + +\nkinds q\nsets A\n", "operator + given twice"));
    CHECK(refused(s + "block x\ncontexts dl\nbinary +\nkinds q q\nsets A\n", "kind q given twice"));
    CHECK(refused(s + "block x\ncontexts dl\nbinary +\nkinds q\nsets A A\n", "value set A given twice"));
    CHECK(refused(s + "values A q=2\n", "value set A: value of kind q given twice"));
    CHECK(refused(s + "block x\ncell A dl + q l\nblock x\ncell A dl - q l\n", "block x given twice"));

    // the structure
    CHECK(refused(s + "contexts dl\n", "contexts outside a block"));
    CHECK(refused(s + "block x\nbinary +\nkinds q\nsets A\n", "block x has no contexts"));
    CHECK(refused(s + "block x\ncontexts dl\nbinary +\nsets A\n", "block x has no kinds"));
    CHECK(refused(s + "block x\ncontexts dl\nkinds q\nsets A\n", "block x has no operators"));
    CHECK(refused(s + "block x\ncontexts dl\nbinary +\nkinds q\n", "block x has no value sets"));
    CHECK(refused(s, "no cells"));
    CHECK(refused(s + "block x\ncell A dl +\n", "cell needs SET CTX OP KIND [KIND]"));
    CHECK(refused(s + "block x\ncell A dl * s s\n", "cell without a meaning: a string under an operator"));
    CHECK(refused(s + "block x\ncell A dl + s s\n", "cell without a meaning: a string + outside print"));
    CHECK(refused(s + "block x\ncell A print = s l\n", "cell without a meaning: a string with a number"));
    CHECK(refused(s + "block a-b\n", "block needs one name"));
    CHECK(refused(s + "cells-per-source 0\n", "cells-per-source needs a number from 1"));
    CHECK(refused(s + "dim x\n", "dim needs a number"));

    // the values
    CHECK(refused(s + "values C b=128\n", "values b=128: outside the Byte range"));
    CHECK(refused(s + "values C w=1.5\n", "values w=1.5: a whole number is needed"));
    CHECK(refused(s + "values C q=40000\n", "outside the Quick range"));
    CHECK(refused(s + "values C cw=100\n", "a constant of Byte size, not of Word size"));
    CHECK(refused(s + "values C cl=1000\n", "a constant of Word size, not of Long size"));
    CHECK(refused(s + "values C cf=2\n", "a constant with a fraction needs a '.'"));
    CHECK(refused(s + "values C f=1e3\n", "not a decimal number"));
    CHECK(refused(s + "values C s=A\"B\n", "a string value holds no"));
    CHECK(refused(s + "counts C s=X\n", "a string has no count"));
    CHECK(refused(s + "values C b\n", "\"b\" is not KIND=VALUE"));
    CHECK(refused("values C q=1.5\nblock x\ncell C dl + q l\n", "m.txt:3: value set C has no value of kind l"));
    CHECK(refused("values C q=1.5\nblock x\ncell C dl LSL q q\n", "value set C has no count of kind q"));

    // all errors are reported, each once
    struct em_matrix m;
    std::string e = parse(s + "block x\ncontexts dl zz\nbinary + %\nkinds q yy\nsets A\n", m, false);
    CHECK(e.find("unknown context \"zz\"") != std::string::npos);
    CHECK(e.find("unknown operator \"%\"") != std::string::npos);
    CHECK(e.find("unknown kind \"yy\"") != std::string::npos);
    e = parse("values C q=1.5\nblock x\ncontexts dl print\nbinary + -\nkinds q l\nsets C\n", m, false);
    CHECK(e == "m.txt:2: value set C has no value of kind l\n");
}

// the generated sources of the description
static std::vector<std::string> generate(const std::string &desc, long n, std::vector<std::string> *names = nullptr,
                                         std::vector<size_t> *counts = nullptr)
{
    struct em_matrix m;
    struct em_source *src = nullptr;
    size_t nsrc = 0;
    char *errors = nullptr;
    std::vector<std::string> v;

    if (parse(desc, m) != "")
        return v;
    int r = em_generate(&m, "m.txt", n, &src, &nsrc, &errors);
    if (r)
        std::fprintf(stderr, "em_generate: %d %s\n", r, errors ? errors : "");
    std::free(errors);
    for (size_t i = 0; i < nsrc; i++) {
        v.push_back(src[i].text);
        if (names)
            names->push_back(src[i].name);
        if (counts)
            counts->push_back(src[i].ncells);
    }
    em_sources_free(src, nsrc);
    em_free(&m);
    return v;
}

TEST_CASE("exprmatrix: the output forms", "[exprmatrix]")
{
    std::string s = SETS;
    std::vector<std::string> v;

    v = generate(s + "block g\ncell A dl * q l\ncell A dl < w cl\n", 0);
    REQUIRE(v.size() == 1);
    CHECK(v[0] == "; g-dl-A-01 - expression matrix: block g, context dl, value set A, cells 1 to 2 of 2\n"
                  "; generated by exprmatrix from m.txt (tools/exprmatrix/README.md); do not edit\n"
                  "vw.w=1000 : vl.l=100000 : vq.q=1.5\n"
                  "rl.l=0\n"
                  "rl=vq * vl : NPrint \"dl-mul-q-l-A \",rl\n"
                  "rl=(vw < 100000) : NPrint \"dl-lt-w-cl-A \",rl\n"
                  "End\n");

    v = generate(s + "block g\ncell B dq LSL f cf\ncell B dq - cb\n", 0);
    REQUIRE(v.size() == 1);
    CHECK(v[0] == "; g-dq-B-01 - expression matrix: block g, context dq, value set B, cells 1 to 2 of 2\n"
                  "; generated by exprmatrix from m.txt (tools/exprmatrix/README.md); do not edit\n"
                  "vf.f=-2.5\n"
                  "rq.q=0\n"
                  "rq=vf LSL 3 : NPrint \"dq-lsl-f-cf-B \",Hex$(Peek.l(&rq))\n"
                  "rq=- -3 : NPrint \"dq-neg-cb-B \",Hex$(Peek.l(&rq))\n"
                  "End\n");

    v = generate(s + "block g\ncell A pw + q w\n", 0);
    REQUIRE(v.size() == 1);
    CHECK(v[0].find("vw.w=1000 : vq.q=1.5\n"
                    "Statement pw{x.w}\n"
                    "  NPrint x\n"
                    "End Statement\n"
                    "Print \"pw-add-q-w-A \" : pw{vq + vw}\n"
                    "End\n") != std::string::npos);

    v = generate(s + "block g\ncell A pf ^ b b\n", 0);
    REQUIRE(v.size() == 1);
    CHECK(v[0].find("vb.b=3\n"
                    "nb.b=2\n"
                    "tf.f=0\n"
                    "Statement pf{x.f}\n"
                    "  SHARED tf\n"
                    "  tf=x : NPrint Hex$(Peek.l(&tf))\n"
                    "End Statement\n"
                    "Print \"pf-pow-b-b-A \" : pf{vb ^ nb}\n") != std::string::npos);

    v = generate(s + "block g\ncell A if AND l q\ncell A if NOT f\n", 0);
    REQUIRE(v.size() == 1);
    CHECK(v[0].find("If vl AND vq Then NPrint \"if-and-l-q-A T\" Else NPrint \"if-and-l-q-A F\"\n"
                    "If NOT vf Then NPrint \"if-not-f-A T\" Else NPrint \"if-not-f-A F\"\n") !=
          std::string::npos);

    v = generate(s + "dim 40\nblock g\ncell A i1 - l w\ncell A i2 / cw b\n", 0);
    REQUIRE(v.size() == 2);
    CHECK(v[0].find("Dim ia.l(40)\nFor ii.l=0 To 40 : ia(ii)=ii*16+1 : Next\n"
                    "NPrint \"i1-sub-l-w-A \",ia(vl - vw)\n") != std::string::npos);
    CHECK(v[1].find("Dim ja.l(1,40)\nFor ii.l=0 To 40 : ja(1,ii)=ii*16+1 : Next\n"
                    "NPrint \"i2-div-cw-b-A \",ja(1,1000 / vb)\n") != std::string::npos);

    v = generate(s + "block g\ncell A print + s s\ncell A print <> s s\ncell A print - cf\n", 0);
    REQUIRE(v.size() == 1);
    CHECK(v[0].find("vs$=\"AB\"\n"
                    "NPrint \"print-add-s-s-A \",vs$ + vs$\n"
                    "NPrint \"print-ne-s-s-A \",vs$ <> vs$\n"
                    "NPrint \"print-neg-cf-A \",-1.5\n") != std::string::npos);

    v = generate(s + "block g\ncell A db EOR cb cw\n", 0);
    REQUIRE(v.size() == 1);
    CHECK(v[0].find("rb.b=0\nrb=3 EOR 1000 : NPrint \"db-eor-cb-cw-A \",rb\n") != std::string::npos);
}

TEST_CASE("exprmatrix: the split at N cells", "[exprmatrix]")
{
    std::string d = std::string(SETS) +
                    "cells-per-source 4\n"
                    "block arith\n"
                    "contexts dl if\n"
                    "binary + -\n"
                    "kinds q l\n"
                    "sets A B\n"
                    "block more\n"
                    "cell A dl * q l\n";
    std::vector<std::string> names;
    std::vector<size_t> counts;
    std::vector<std::string> v = generate(d, 0, &names, &counts);

    std::vector<std::string> want = { "arith-dl-A-01", "arith-dl-A-02", "arith-if-A-01", "arith-if-A-02",
                                      "arith-dl-B-01", "arith-dl-B-02", "arith-if-B-01", "arith-if-B-02",
                                      "more-dl-A-01" };
    CHECK(names == want);
    CHECK(counts == std::vector<size_t>{ 4, 4, 4, 4, 4, 4, 4, 4, 1 });
    REQUIRE(v.size() == 9);
    CHECK(v[1].find("cells 5 to 8 of 8\n") != std::string::npos);
    CHECK(v[1].find("rl=vl + vq : NPrint \"dl-add-l-q-A \"") == std::string::npos);
    CHECK(v[1].find("rl=vq - vq : NPrint \"dl-sub-q-q-A \",rl\n") != std::string::npos);

    // the option overrides the description; a group smaller than N is one source
    names.clear();
    counts.clear();
    v = generate(d, 100, &names, &counts);
    CHECK(names == std::vector<std::string>{ "arith-dl-A-01", "arith-if-A-01", "arith-dl-B-01", "arith-if-B-01",
                                             "more-dl-A-01" });
    CHECK(counts == std::vector<size_t>{ 8, 8, 8, 8, 1 });

    // every key in exactly one source
    names.clear();
    v = generate(d, 3, &names);
    size_t lines = 0;
    for (const std::string &t : v)
        for (size_t p = t.find("NPrint \""); p != std::string::npos; p = t.find("NPrint \"", p + 1))
            lines++;
    CHECK(lines == 49);         // the 16 if cells print in two places
    CHECK(names.size() == 13);
}

TEST_CASE("exprmatrix: a source name too long", "[exprmatrix]")
{
    struct em_matrix m;
    struct em_source *src = nullptr;
    size_t nsrc = 0;
    char *errors = nullptr;
    std::string d = std::string(SETS) + "block averyveryverylongname\ncell A print + q l\n";

    REQUIRE(parse(d, m) == "");
    CHECK(em_generate(&m, "m.txt", 0, &src, &nsrc, &errors) == 1);
    CHECK(src == nullptr);
    CHECK(nsrc == 0);
    REQUIRE(errors != nullptr);
    CHECK(std::string(errors) ==
          "averyveryverylongname-print-A-01: source name longer than 25 characters (block "
          "averyveryverylongname, line 5)\n");
    std::free(errors);
    em_free(&m);
}
