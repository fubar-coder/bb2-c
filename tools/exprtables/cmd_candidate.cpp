/*
 * exprtables candidate - the tables of a candidate rule of the decision
 * round of expr-tree-5-operand-typing (open question 1; exprtype_tables.h,
 * ett_rules_cand, ett_page_opts): the same tables as today's page, the
 * cells that differ from today's marked, and the cells of the CD places
 * the candidate retypes (a marks file) marked.
 */
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <set>
#include <string>
#include "cli.hpp"
extern "C" {
#include "exprmatrix.h"
#include "exprtype_tables.h"
}
#include "cmds.h"

namespace {

bool read_file(const std::string &path, std::string &text)
{
    FILE *f = std::fopen(path.c_str(), "rb");
    char buf[8192];
    size_t n;

    if (!f)
        return false;
    text.clear();
    while ((n = std::fread(buf, 1, sizeof buf, f)) > 0)
        text.append(buf, n);
    bool ok = !std::ferror(f);
    std::fclose(f);
    return ok;
}

bool write_file(const std::string &path, const std::string &text)
{
    FILE *f = std::fopen(path.c_str(), "wb");

    if (!f)
        return false;
    bool ok = std::fwrite(text.data(), 1, text.size(), f) == text.size();
    return std::fclose(f) == 0 && ok;
}

// what each candidate is (exprmatrix.h, model.c "Candidate rules")
const char *const about_common =
    "This page is not a description of the compiler: it shows a *candidate* rule of the\n"
    "decision round of the OpenSpec change `expr-tree-5-operand-typing` (open question 1),\n"
    "computed by the expression matrix's model (`tools/lib/exprmatrix/model.c`,\n"
    "`exprmatrix candidates`), as the same tables as [today's\n"
    "page](expression-type-tables.md).\n"
    "\n"
    "With operand typing there is no type so far: an operation is typed by its operator,\n"
    "the types of its two operands and the *wanted type* (the destination's or\n"
    "parameter's type, Long for an index, none for `If`, `Print` and the operands of a\n"
    "comparison), which reaches every operand from the top. So a column is the wanted\n"
    "type at the operation, the same in every place of an expression, and the left and\n"
    "the right operand are the values of the subexpressions (a variable, or an operation\n"
    "whose result type is its row's type). A cell `L R \xe2\x86\x92 T` says: the left operand is taken\n"
    "as type L, the right one as type R, and the operation's result has type T. `Q~` is a\n"
    "Quick taken with its fraction into an exact operation whose result is rounded down\n"
    "at the operator (#1178). \xe2\x86\x93 marks a Quick or Float converted down.\n"
    "\n"
    "Marks: \xe2\x97\x86 the cell differs from today's (today's column is the type so far at the left\n"
    "operand, which is the wanted type for the first operation of an expression);\n"
    "\xe2\x80\xa1 operations of two variables or function values of this cell are in CD sources whose\n"
    "typing the candidate changes (the count of the decision round); \xe2\x80\xa0 the cell differs\n"
    "from the operation with its operands swapped (none is expected).\n"
    "\n"
    "The tables with a comparison as the right operand (#1182) and of a later index\n"
    "(#1179), the bit test operators and a constant alone are today's: the model does not\n"
    "type them.\n";

const char *const about_c[EM_NCAND] = {
    "",
    "**C1, symmetric flow.** Constants take the type today's flow gives them as the second\n"
    "operand: the higher of the wanted type and the other operand's type (beside a Quick\n"
    "under a wanted Long a Long). The wanted type reaches every Quick as today: under a\n"
    "wanted Long every operation with a Quick is exact and rounded down (#1178 variant C),\n"
    "under a wanted Byte or Word a Quick operation stays a Quick one. Word+Word is a Long\n"
    "as today (the add of two Words, then a Long), so `w+v+q` still differs from `w-v+q`.\n"
    "A Long and a Quick under no wanted type: exact, rounded down to a Long (today's\n"
    "`l+q`; today's `q+l` was a Quick).\n",
    "**C2, own types.** Constants take the other operand's type (a constant with a\n"
    "fraction a Quick, a Float beside a Float). A wanted whole number does not reach a\n"
    "Quick: a Quick operation stays a Quick one under a wanted Long too (its range\n"
    "\xc2\xb1" "32768), only a Long and a Quick are exact. Word+Word and Word-Word are a Word (they\n"
    "wrap), Word*Word a Long.\n",
    "**C3, value classes.** Constants by their value: a whole number a Byte, Word or Long\n"
    "by its size, a fraction a Quick (a Float beside a Float or under a wanted Float).\n"
    "Under a wanted whole number an operation of a Quick and a whole number is exact,\n"
    "Quick with Quick stays a Quick one. Bytes and Words are added and subtracted in Long\n"
    "(no wrap).\n",
    "**C4, integral and fractional numbers** (the user, 2026-11-24). C1 with constants as\n"
    "generic numbers: a whole value takes the other operand's type if it fits (beside a\n"
    "Quick under a wanted whole number a Long, the Quick-times-Long operation), a value\n"
    "with a fraction is a Quick (a Float beside a Float or under a wanted Float).\n",
    "**C5, 32.16 products** (the user, 2026-11-24). C1, and the product of a Quick and a\n"
    "Byte, Word or Long (either order) is an exact 32.16 value (48.16 for a Long, the\n"
    "overflow of its 32-bit whole part seen) that its consumer converts: a wanted whole\n"
    "number takes its whole part (a Word its low word), a wanted Quick or none 16.16 (none\n"
    "with a Long operand: the whole part). The cells of such a product show it as\n"
    "`Q~ W \xe2\x86\x92 L` (its result for a whole-number consumer) or `\xe2\x86\x92 Q`. The overflow is cut, or\n"
    "with runtime error checking a runtime error; the tables are the same for both.\n",
    "**C6, 32.16 products with whole constants by value** (task 2.1a). C5, and a whole\n"
    "constant beside a Quick in a `*` takes its value class (a Byte, Word or Long by its\n"
    "size, C3's rule; a constant with a fraction as in C1), so `q*1000` is a 32.16 product\n"
    "as `q*w` is. The cells of two variables are C5's; the difference is in the constants.\n"
    "The overflow is cut, or with runtime error checking a runtime error.\n",
    "**C7, the wanted type sets the intermediate format** (the user, 2026-11-24). In an\n"
    "expression with a Quick every intermediate result has the format the wanted type\n"
    "sets, rounded once at the consumer: a wanted Long 32.16 (a register pair), a wanted\n"
    "Quick, Word or Byte 16.16 (the subtree of a `/` or `Mod` with a Long in it 32.16), no\n"
    "wanted type 32.16 where a Quick meets a whole value or constant, 16.16 for Quick with\n"
    "Quick. An expression without a Quick, with a Float or under a wanted Float is today's.\n"
    "A cell `Q~ L \xe2\x86\x92 L` with a Quick is a 32.16 operation, `Q Q \xe2\x86\x92 Q` a 16.16 one; the result\n"
    "letter is the format (L for 32.16), not a type the operand keeps: it is converted\n"
    "once, at the consumer.\n",
};

const char *const notes =
    "## Notes\n"
    "\n"
    "* The left operand of a comparison: one element keeps its own type in every\n"
    "  candidate (as today, #1177). An operation on the left is typed by its own operands\n"
    "  (scope (a)), or under scope (b) only when a Quick or Float is inside it or it is a\n"
    "  Long constant under a wanted Float, else under the wanted type (#1183). The cells\n"
    "  of the comparisons are two variables, so they are the same for both scopes.\n"
    "* `&`, `|`, `AND`, `OR`, `EOR` on a Quick operation are on its whole parts (#1209),\n"
    "  as today.\n"
    "* A later array index is a Long (#1179), as today.\n";

struct marks {
    std::set<std::string> keys;     // "op W l r" in the letters of exprmatrix candidates --typing
};

const char *mop(unsigned op)
{
    switch (op) {
    case ETO_ADD: return "add";
    case ETO_SUB: return "sub";
    case ETO_MUL: return "mul";
    case ETO_DIV: return "div";
    case ETO_AMP:
    case ETO_AND: return "and";
    case ETO_BAR:
    case ETO_OR: return "or";
    case ETO_EOR: return "eor";
    case ETO_LSL: return "lsl";
    case ETO_ASL: return "asl";
    case ETO_LSR: return "lsr";
    case ETO_ASR: return "asr";
    case ETO_MOD: return "mod";
    case ETO_POW: return "pow";
    case ETO_EQ: return "eq";
    case ETO_NE: return "ne";
    case ETO_LT: return "lt";
    case ETO_LE: return "le";
    case ETO_GT: return "gt";
    case ETO_GE: return "ge";
    default: return "";
    }
}

int mark(void *ctx, unsigned op, unsigned want, unsigned l, unsigned r)
{
    const marks *m = static_cast<const marks *>(ctx);
    static const char letters[] = "-BWLQFAS";
    std::string k = std::string(mop(op)) + " " + letters[want & 7] + " " + letters[l & 7] + " " + letters[r & 7];

    return m->keys.count(k) ? 1 : 0;
}

int beside(void *ctx, unsigned want, unsigned partner, const char *text)
{
    return em_cand_consttype(*static_cast<uint64_t *>(ctx), (int)want, (int)partner, text);
}

const char *const about_c7k =
    "\n**C7K** (task 2.1b, \"C7: constants by their smallest type\", 2026-11-24): C7, with every\n"
    "constant by its smallest type (Byte, Word, Long; a fraction a Quick), adapted where the\n"
    "operation needs it, and the operands of every comparison typed by their own operands,\n"
    "also without a Quick. A whole-number operation in a comparison's operand: ";

int run(const std::string &me, const std::string &name, bool scopeb, const std::string &marksfile,
        const std::string &out, const std::string &c7k)
{
    int n;
    for (n = EM_CAND_C1; n < EM_NCAND; n++)
        if (name == em_cand_name(n))
            break;
    if (n == EM_NCAND) {
        std::fprintf(stderr, "%s: no candidate %s (C1 to C%d)\n", me.c_str(), name.c_str(), EM_NCAND - 1);
        return 2;
    }
    marks m;
    if (!marksfile.empty()) {
        std::string text;
        if (!read_file(marksfile, text)) {
            std::fprintf(stderr, "%s: %s: %s\n", me.c_str(), marksfile.c_str(), std::strerror(errno));
            return 1;
        }
        size_t a = 0;
        while (a < text.size()) {
            size_t b = text.find('\n', a);
            std::string line = text.substr(a, b == std::string::npos ? std::string::npos : b - a);
            a = b == std::string::npos ? text.size() : b + 1;
            if (!line.empty() && line[0] != '#')
                m.keys.insert(line.substr(0, line.find('\t')));
        }
    }
    struct ett_prims p;
    struct ett_rules base, r;
    struct ett_cand c;
    uint64_t rules = EM_RULES_CANDBASE | EM_R_CAND(n) | (scopeb ? EM_R_SCOPEB : 0);
    std::string k7;

    if (!c7k.empty()) {
        if (n != EM_CAND_C7 || (c7k != "own" && c7k != "long" && c7k != "dest" && c7k != "x")) {
            std::fprintf(stderr, "%s: --c7k own, long, dest or x, with C7\n", me.c_str());
            return 2;
        }
        rules |= EM_R_C7K | (c7k == "long"   ? EM_R_C7KLONG
                             : c7k == "dest" ? EM_R_C7KDEST
                             : c7k == "x"    ? EM_R_C7KLONG | EM_R_C7KEXACT
                                             : 0);
        k7 = std::string(about_c7k) +
             (c7k == "own" ? "in its own type (C7K-own).\n"
              : c7k == "long" ? "at least a Long (C7K-long).\n"
              : c7k == "x"    ? "at least a Long; and a constant with a fraction is a Quick only if a 16.16 holds "
                                "it exactly or a Quick is beside it, else a Float as today (C7Kx, the refinement).\n"
                              : "at least the destination's whole-number width (r.l a Long, r.w a Word, rb.b a "
                                "Byte; a Quick or Float destination and none: its own type) (C7K-dest).\n");
    }

    ett_prims_exprtype(&p);
    ett_rules_flow(&base, &p);
    c.rules = rules;
    c.base = &base;
    ett_rules_cand(&r, &c);
    std::string title = (c7k.empty() ? std::string("Candidate ") + em_cand_name(n) + ": " + em_cand_title(n)
                                     : std::string("Candidate C7K-") + c7k + ": " + em_cand_title(n) +
                                           ", constants by their smallest type") +
                        (scopeb ? ", scope (b)" : "");
    std::string about = std::string(about_common) + "\n" + about_c[n] + k7;
    struct ett_pageopts o = { title.c_str(), about.c_str(), notes, &base, marksfile.empty() ? nullptr : mark, &m,
                              beside, &rules, nullptr, nullptr, nullptr, nullptr, nullptr };
    struct ett_stats st;
    char *error = nullptr;
    char *page = ett_page_opts(&r, &o, &st, &error);

    if (!page) {
        std::fprintf(stderr, "%s: %s\n", me.c_str(), error ? error : "out of memory");
        std::free(error);
        return 1;
    }
    static const char *const groups[ETG_COUNT] = { "arithmetic", "bit operators", "shifts", "Mod",
                                                   "power", "comparisons", "bit test and set" };
    unsigned ch = 0, mk = 0;
    for (int g = 0; g < ETG_COUNT; g++) {
        if (!out.empty())
            std::printf("%s: %u cells changed, %u marked\n", groups[g], st.changed[g], st.marked[g]);
        ch += st.changed[g];
        mk += st.marked[g];
    }
    if (!out.empty())
        std::printf("all: %u cells changed, %u marked\n", ch, mk);
    int status = 0;
    if (!out.empty() && !write_file(out, page)) {
        std::fprintf(stderr, "%s: %s: %s\n", me.c_str(), out.c_str(), std::strerror(errno));
        status = 1;
    } else if (out.empty())
        std::fputs(page, stdout);
    std::free(page);
    return status;
}

}  // namespace

void cmd_candidate(CLI::App &app, int &status)
{
    struct options {
        std::string name, marks, out, c7k;
        bool scopeb = false;
    };
    auto o = std::make_shared<options>();
    CLI::App *sub = app.add_subcommand("candidate", "the tables of a candidate rule of expr-tree-5 beside today's");
    sub->add_option("NAME", o->name, "the candidate, C1 to C7 (exprmatrix candidates)")->required();
    sub->add_flag("--scope-b", o->scopeb, "the left operand of a comparison by scope (b), not (a)");
    sub->add_option("--marks", o->marks, "the cells of the CD places to mark: OP WANT LEFT RIGHT per line")
        ->type_name("FILE");
    sub->add_option("-o,--out", o->out, "write the page to FILE (default: stdout)")->type_name("FILE");
    sub->add_option("--c7k", o->c7k, "C7 as C7K: own, long, dest or x (a comparison's whole-number operations)")
        ->type_name("VARIANT");
    sub->footer("With --out prints the cells that differ from today's and the marked ones per operator group. A line of "
                "--marks is the cell as exprmatrix candidates --typing writes it (\"mul L W Q\"), anything after "
                "a tab ignored, # starts a comment. Exit status 0, 1 a file cannot be read or written, 2 usage "
                "error or no such candidate.");
    sub->callback([sub, o, &status] { status = run(cli::name(*sub), o->name, o->scopeb, o->marks, o->out, o->c7k); });
}
