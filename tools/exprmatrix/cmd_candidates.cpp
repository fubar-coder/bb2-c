/*
 * exprmatrix candidates - the candidate rules of the decision round of
 * expr-tree-5-operand-typing (exprmatrix.h, EM_R_CAND): the cells of the
 * matrix whose value a candidate changes against the port's rules, and
 * the values of expressions given as text under today's rules and each
 * candidate.
 */
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "cli.hpp"
extern "C" {
#include "exprmatrix.h"
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

// a rule set of the comparison: candidate n with scope (a) or (b)
struct ruleset {
    std::string name;
    uint64_t rules;
};

std::vector<ruleset> rulesets()
{
    std::vector<ruleset> v;

    v.push_back({ "flow", EM_RULES_CANDBASE });
    for (int n = EM_CAND_C1; n < EM_NCAND; n++) {
        v.push_back({ std::string(em_cand_name(n)) + "a", EM_RULES_CANDBASE | EM_R_CAND(n) });
        v.push_back({ std::string(em_cand_name(n)) + "b", EM_RULES_CANDBASE | EM_R_CAND(n) | EM_R_SCOPEB });
    }
    // C5 with the overflow of its 32.16 product a runtime error
    v.push_back({ "C5ea", EM_RULES_CANDBASE | EM_R_CAND(EM_CAND_C5) | EM_R_C5ERR });
    v.push_back({ "C5eb", EM_RULES_CANDBASE | EM_R_CAND(EM_CAND_C5) | EM_R_C5ERR | EM_R_SCOPEB });
    v.push_back({ "C6ea", EM_RULES_CANDBASE | EM_R_CAND(EM_CAND_C6) | EM_R_C5ERR });
    v.push_back({ "C6eb", EM_RULES_CANDBASE | EM_R_CAND(EM_CAND_C6) | EM_R_C5ERR | EM_R_SCOPEB });
    // C7K: constants by their smallest type, comparisons by their operands
    v.push_back({ "C7Kown", EM_RULES_CANDBASE | EM_R_CAND(EM_CAND_C7) | EM_R_C7K });
    v.push_back({ "C7Klong", EM_RULES_CANDBASE | EM_R_CAND(EM_CAND_C7) | EM_R_C7K | EM_R_C7KLONG });
    v.push_back({ "C7Kdest", EM_RULES_CANDBASE | EM_R_CAND(EM_CAND_C7) | EM_R_C7K | EM_R_C7KDEST });
    v.push_back({ "C7Kx", EM_RULES_CANDBASE | EM_R_CAND(EM_CAND_C7) | EM_R_C7K | EM_R_C7KLONG | EM_R_C7KEXACT });
    // the analysis's P (changes 1-6) and the decided rule P+ (changes 1-8)
    v.push_back({ "P", EM_RULES_P });
    v.push_back({ "P+", EM_RULES_DECIDED });
    // the rules of the OpenSpec change type-system-rework
    v.push_back({ "TSR", EM_RULES_TSR });
    return v;
}

const char *group_of(enum em_op op)
{
    switch (op) {
    case EM_ADD: case EM_SUB: case EM_MUL: case EM_DIV: return "arith";
    case EM_MOD: return "mod";
    case EM_POW: return "pow";
    case EM_AND: case EM_OR: case EM_EOR: case EM_NOT: return "bit";
    case EM_LSL: case EM_LSR: case EM_ASL: case EM_ASR: return "shift";
    case EM_NEG: return "neg";
    default: return "compare";
    }
}

std::string value_of(const struct em_matrix *m, const struct em_cell *c, uint64_t rules)
{
    struct em_value v, vc;

    em_model(m, c, rules, 0, &v);
    if (c->ctx != EM_I1 && c->ctx != EM_I2)
        return v.text;
    em_model(m, c, rules, 1, &vc);
    return std::string(v.text) + "/" + vc.text;
}

int cells(const std::string &me, const std::string &desc, const std::string &out, std::string &report)
{
    std::string dtext;
    struct em_matrix m;
    char *errors = nullptr;
    auto sets = rulesets();

    if (!read_file(desc, dtext)) {
        std::fprintf(stderr, "%s: %s: %s\n", me.c_str(), desc.c_str(), std::strerror(errno));
        return 1;
    }
    int r = em_parse(dtext.data(), dtext.size(), desc.c_str(), &m, &errors);
    if (r) {
        std::fprintf(stderr, "%s", r < 0 ? "" : errors);
        if (r < 0)
            std::fprintf(stderr, "%s: out of memory\n", me.c_str());
        std::free(errors);
        return 1;
    }
    std::string list = "key\tcontext\tgroup";
    for (auto &s : sets)
        list += "\t" + s.name;
    list += "\n";
    // [set][what] -> count
    std::map<std::string, std::map<std::string, size_t>> count;
    for (size_t i = 0; i < m.ncells; i++) {
        const struct em_cell *c = &m.cells[i];
        std::vector<std::string> v;
        bool any = false;

        for (auto &s : sets)
            v.push_back(value_of(&m, c, s.rules));
        for (size_t k = 1; k < sets.size(); k++)
            if (v[k] != v[0]) {
                bool was = v[0].find("!noncompile") != std::string::npos;
                bool now = v[k].find("!noncompile") != std::string::npos;

                any = true;
                count[sets[k].name]["all"]++;
                count[sets[k].name][was ? "kind compiles now" : now ? "kind refused now" : "kind value"]++;
                count[sets[k].name][std::string("context ") + em_ctx_name(c->ctx)]++;
                count[sets[k].name][std::string("group ") + group_of(c->op)]++;
                count[sets[k].name][std::string(em_op_is_compare(c->op) ? "#1183 (comparisons)" : "#1180 (order)")]++;
            }
        if (!any)
            continue;
        list += std::string(c->key) + "\t" + em_ctx_name(c->ctx) + "\t" + group_of(c->op);
        for (auto &x : v)
            list += "\t" + x;
        list += "\n";
    }
    report += "cells: " + std::to_string(m.ncells) + "\n";
    for (size_t k = 1; k < sets.size(); k++) {
        auto &cc = count[sets[k].name];
        report += sets[k].name + ": " + std::to_string(cc["all"]) + " changed";
        for (auto &[what, n] : cc)
            if (what != "all")
                report += "; " + what + " " + std::to_string(n);
        report += "\n";
    }
    em_free(&m);
    if (!out.empty() && !write_file(out, list)) {
        std::fprintf(stderr, "%s: %s: %s\n", me.c_str(), out.c_str(), std::strerror(errno));
        return 1;
    }
    return 0;
}

bool ctx_by_name(const std::string &s, enum em_ctx *x)
{
    for (int i = 0; i < EM_NCTX; i++)
        if (s == em_ctx_name((enum em_ctx)i)) {
            *x = (enum em_ctx)i;
            return true;
        }
    return false;
}

std::vector<std::string> split_tabs(const std::string &line)
{
    std::vector<std::string> f;
    size_t a = 0;

    for (;;) {
        size_t b = line.find('\t', a);
        f.push_back(line.substr(a, b == std::string::npos ? std::string::npos : b - a));
        if (b == std::string::npos)
            return f;
        a = b + 1;
    }
}

int exprs(const std::string &me, const std::string &in, const std::string &out, bool sigs)
{
    std::string text, res;
    auto sets = rulesets();
    int bad = 0, lineno = 0;

    // the expressions also by the original's rules (blitz2 1.95, the CD's
    // libraries), the column after flow
    sets.insert(sets.begin() + 1, ruleset{ "orig", EM_RULES_ORIG });

    if (!read_file(in, text)) {
        std::fprintf(stderr, "%s: %s: %s\n", me.c_str(), in.c_str(), std::strerror(errno));
        return 1;
    }
    res = "id\tcontext\texpression";
    for (auto &s : sets)
        res += "\t" + s.name;
    if (sigs) {
        for (auto &s : sets)
            res += "\ttyping " + s.name;
        for (auto &s : sets)
            res += "\tcells " + s.name;
    }
    res += "\n";
    size_t a = 0;
    while (a < text.size()) {
        size_t b = text.find('\n', a);
        std::string line = text.substr(a, b == std::string::npos ? std::string::npos : b - a);
        a = b == std::string::npos ? text.size() : b + 1;
        lineno++;
        if (line.empty() || line[0] == '#')
            continue;
        auto f = split_tabs(line);
        enum em_ctx ctx;
        struct em_expr *x = nullptr;
        char *err = nullptr;

        if (f.size() < 3 || !ctx_by_name(f[1], &ctx)) {
            std::fprintf(stderr, "%s:%d: ID, CONTEXT, EXPRESSION and VALUES expected\n", in.c_str(), lineno);
            bad++;
            continue;
        }
        int r = em_expr_parse(f[2].c_str(), f.size() > 3 ? f[3].c_str() : "", &x, &err);
        if (r) {
            std::fprintf(stderr, "%s:%d: %s\n", in.c_str(), lineno, r < 0 ? "out of memory" : err);
            std::free(err);
            bad++;
            continue;
        }
        std::vector<std::string> sig, cells;
        res += f[0] + "\t" + f[1] + "\t" + f[2];
        for (auto &s : sets) {
            struct em_exprval v;

            em_expr_eval(x, ctx, s.rules, &v);
            res += std::string("\t") + v.v.text;
            if (v.v.flags & EM_F_OVF)
                res += "!";
            sig.push_back(v.sig);
            cells.push_back(v.cells);
        }
        if (sigs) {
            for (auto &s : sig)
                res += "\t" + s;
            for (auto &s : cells)
                res += "\t" + s;
        }
        res += "\n";
        em_expr_free(x);
    }
    if (!write_file(out, res)) {
        std::fprintf(stderr, "%s: %s: %s\n", me.c_str(), out.c_str(), std::strerror(errno));
        return 1;
    }
    return bad ? 1 : 0;
}

}  // namespace

void cmd_candidates(CLI::App &app, int &status)
{
    struct options {
        std::string desc, out, exprs, values;
        bool typing = false;
    };
    auto o = std::make_shared<options>();
    CLI::App *sub = app.add_subcommand("candidates", "the candidate rules of expr-tree-5: changed cells and values");

    sub->add_option("DESC", o->desc, "the description of the matrix")->required()->type_name("FILE");
    sub->add_option("-o,--out", o->out, "the cells a candidate changes, with every rule set's value (written)")
        ->type_name("CELLS");
    sub->add_option("-e,--exprs", o->exprs, "expressions to evaluate: ID, CONTEXT, EXPRESSION, VALUES per line")
        ->type_name("FILE");
    sub->add_option("-v,--values", o->values, "their values under every rule set (written)")->type_name("FILE");
    sub->add_flag("--typing", o->typing, "with the typing and the table cell of each operation beside the values");
    sub->footer("The rule sets: flow (the port's rules) and each candidate C1 to C7 with the left operand "
                "of a comparison by scope (a) and (b) (exprmatrix.h, EM_R_CAND), and the decided rule P+ "
                "with P, its changes 1-6 (EM_RULES_DECIDED, EM_RULES_P), and TSR, the rules of "
                "type-system-rework (EM_RULES_TSR). Prints the number of "
                "cells each one changes, per context, operator group and issue. With --exprs each line "
                "(tab separated; # starts a comment) is evaluated under each rule set into --values. "
                "Exit status 0, 1 a file cannot be read or written or is malformed (one message per "
                "error), 2 usage error.");
    sub->callback([sub, o, &status] {
        std::string me = cli::name(*sub), report;

        if (!o->exprs.empty() && o->values.empty()) {
            std::fprintf(stderr, "%s: --exprs needs --values\n", me.c_str());
            status = 2;
            return;
        }
        status = cells(me, o->desc, o->out, report);
        if (!status)
            std::fputs(report.c_str(), stdout);
        if (!status && !o->exprs.empty())
            status = exprs(me, o->exprs, o->values, o->typing);
    });
}
