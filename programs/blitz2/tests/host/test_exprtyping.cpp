// test_exprtyping - the typing pass of rule P+ (programs/blitz2/src/
// exprtyping.c; change expr-tree-5-operand-typing, task 3.3): every
// decision of the pass against the expression matrix's model
// (tools/lib/exprmatrix/model.c, EM_RULES_DECIDED, an implementation of its
// own): per operation in post order its path (today's handler, a
// comparison, an exact comparison, 16.16, 48.16, #1178's routines), the
// type each operand comes from, the type it is taken in and whether it is
// taken as 48.16, the result type (the model's typing signature, sig), the
// refusal, and how the value of a fractional expression is taken (the
// rounding point, the model's consumer).  For every cell of the matrix
// (tests/corpus/tests-exprmatrix/matrix.txt) but the index and string
// ones, and for the chains of the analysis
// (~/.local/share/bb2-c-work/expr-tree-5-analysis/report.md: the order
// pairs of conflicts 12-15, the idioms of 1-4 and the other conflicts),
// the spec's scenarios and the decided cases of the model's tests, each in
// every context.  The fast path of a 48.16 operation is checked with
// exprarith.c's test of a narrow value.
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <functional>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#include <snitch/snitch.hpp>
extern "C" {
#include "exprarith.h"
#include "exprmatrix.h"
#include "exprtyping.h"
}

namespace {

const uint64_t DEC = EM_RULES_DECIDED;

const enum em_ctx CONTEXTS[] = { EM_DB, EM_DW, EM_DL, EM_DQ, EM_DF, EM_PL, EM_PW, EM_PQ, EM_PF, EM_IF, EM_PRINT };

unsigned want_of(enum em_ctx ctx)
{
    static const unsigned t[] = { ET_BYTE, ET_WORD, ET_LONG, ET_QUICK, ET_FLOAT, ET_LONG, ET_WORD,
                                  ET_QUICK, ET_FLOAT, ET_NONE, ET_LONG, ET_LONG, ET_NONE };
    return t[ctx];
}

// ------------------------------------------------------------ the tree

// an expression as a tree of exprtyping nodes, parsed as the model parses
// it (em_expr_parse: blitz2's precedences, brackets without a node of
// their own, NOT over the rest, a minus before an element its NEG)
struct tree {
    std::vector<exprtyping_node> nodes;
    std::map<std::string, unsigned> vars;
    exprtyping_node *root = nullptr;
    const char *p = nullptr;
    std::string err;

    tree(const std::string &text, const std::string &varlist)
    {
        nodes.reserve(512);
        std::istringstream vs(varlist);
        std::string v;
        while (vs >> v) {
            size_t eq = v.find('='), dot = v.find('.');
            unsigned t = ET_QUICK;

            if (dot != std::string::npos && dot < eq)
                t = type(v[dot + 1]);
            vars[v.substr(0, std::min(dot, eq))] = t;
        }
        p = text.c_str();
        root = expr(0);
        skip();
        if (root && *p)
            fail("an operator expected");
        if (!err.empty())
            root = nullptr;
    }
    static unsigned type(char c)
    {
        switch (c | 0x20) {
        case 'b': return ET_BYTE;
        case 'w': return ET_WORD;
        case 'l': return ET_LONG;
        case 'q': return ET_QUICK;
        default: return ET_FLOAT;
        }
    }
    exprtyping_node *fail(const char *m)
    {
        if (err.empty())
            err = std::string(m) + " at \"" + p + "\"";
        return nullptr;
    }
    exprtyping_node *node(unsigned kind)
    {
        if (nodes.size() == nodes.capacity())
            return fail("too many nodes");
        nodes.push_back(exprtyping_node{});
        nodes.back().kind = (unsigned char)kind;
        return &nodes.back();
    }
    void skip()
    {
        while (*p == ' ')
            p++;
    }
    static bool idc(char c)
    {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
    }
    bool kw(const char *k)
    {
        size_t n = std::strlen(k);

        for (size_t i = 0; i < n; i++)
            if ((p[i] | 0x20) != (k[i] | 0x20))
                return false;
        if (idc(p[n]))
            return false;
        p += n;
        return true;
    }
    int binop(unsigned *op, bool peek)
    {
        static const struct {
            const char *t;
            unsigned op;
            int prec;
            bool kw;
        } ops[] = {
            { "AND", ETO_AND, 1000, true }, { "OR", ETO_OR, 1000, true },   { "EOR", ETO_EOR, 1000, true },
            { "MOD", ETO_MOD, 5000, true }, { "LSL", ETO_LSL, 6000, true }, { "ASL", ETO_ASL, 6000, true },
            { "LSR", ETO_LSR, 6000, true }, { "ASR", ETO_ASR, 6000, true }, { "<>", ETO_NE, 2000, false },
            { "><", ETO_NE, 2000, false },  { "<=", ETO_LE, 2000, false },  { "=<", ETO_LE, 2000, false },
            { ">=", ETO_GE, 2000, false },  { "=>", ETO_GE, 2000, false },  { "=", ETO_EQ, 2000, false },
            { "<", ETO_LT, 2000, false },   { ">", ETO_GT, 2000, false },   { "+", ETO_ADD, 3000, false },
            { "-", ETO_SUB, 3000, false },  { "*", ETO_MUL, 4000, false },  { "/", ETO_DIV, 4000, false },
            { "&", ETO_AMP, 5000, false },  { "|", ETO_BAR, 5000, false },  { "^", ETO_POW, 7000, false },
        };
        const char *save;

        skip();
        save = p;
        for (const auto &o : ops) {
            bool hit = o.kw ? kw(o.t) : !std::strncmp(p, o.t, std::strlen(o.t));

            if (hit) {
                if (!o.kw)
                    p += std::strlen(o.t);
                *op = o.op;
                if (peek)
                    p = save;
                return o.prec;
            }
        }
        return 0;
    }
    exprtyping_node *number()
    {
        exprtyping_node *n = node(ETN_CONST);
        int64_t num = 0, den = 1;

        if (!n)
            return nullptr;
        if (*p == '$' || *p == '%') {
            char *end;
            int32_t s = (int32_t)(uint32_t)std::strtoul(p + 1, &end, *p == '$' ? 16 : 2);

            p = end;
            n->den = 1;
            if (s < 0) {
                exprtyping_node *m = node(ETN_NEG);

                n = &nodes[nodes.size() - 2];
                n->num = -(int64_t)s;
                m->l = n;
                return m;
            }
            n->num = s;
            return n;
        }
        while ((*p >= '0' && *p <= '9') || *p == '.') {
            if (*p == '.')
                n->point = 1;
            else {
                num = num * 10 + (*p - '0');
                if (n->point)
                    den *= 10;
            }
            p++;
        }
        n->num = num;
        n->den = den;
        return n;
    }
    exprtyping_node *element()
    {
        exprtyping_node *n;

        skip();
        if (*p == '-') {
            p++;
            n = node(ETN_NEG);
            size_t at = nodes.size() - 1;
            exprtyping_node *l = element();
            if (!n || !l)
                return nullptr;
            nodes[at].l = l;
            return &nodes[at];
        }
        if (kw("NOT")) {
            n = node(ETN_NOT);
            size_t at = nodes.size() - 1;
            exprtyping_node *l = expr(0);
            if (!n || !l)
                return nullptr;
            nodes[at].l = l;
            return &nodes[at];
        }
        if (*p == '(') {
            p++;
            n = expr(0);
            skip();
            if (!n)
                return nullptr;
            if (*p != ')')
                return fail("')' expected");
            p++;
            return n;
        }
        if ((*p >= '0' && *p <= '9') || *p == '.' || *p == '$' || *p == '%')
            return number();
        if (idc(*p)) {
            std::string name;
            unsigned t = 0;

            while (idc(*p))
                name += *p++;
            if (*p == '.' && std::strchr("bwlqfBWLQF", p[1]) && p[1] && !idc(p[2])) {
                t = type(p[1]);
                p += 2;
            }
            auto it = vars.find(name);
            if (it == vars.end())
                return fail("no variable");
            n = node(ETN_VAR);
            if (n)
                n->type = (unsigned char)(t ? t : it->second);
            return n;
        }
        return fail("an element expected");
    }
    exprtyping_node *expr(int minprec)
    {
        exprtyping_node *l = element();
        unsigned op;
        int prec;

        while (l && (prec = binop(&op, true)) > minprec) {
            binop(&op, false);
            exprtyping_node *n = node(ETN_BIN);
            if (!n)
                return nullptr;
            size_t at = nodes.size() - 1;
            nodes[at].op = (unsigned char)op;
            nodes[at].l = l;
            exprtyping_node *r = expr(prec);
            if (!r)
                return nullptr;
            nodes[at].r = r;
            l = &nodes[at];
        }
        return l;
    }
};

// ------------------------------------------------------- the rendering

const char *opkey(unsigned op)
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
    default: return "ge";
    }
}

char tlet(unsigned t, unsigned wide)
{
    if (wide)
        return '~';
    if (t == ETY_CONST)
        return 'k';
    return "-BWLQFAS"[t <= 7 ? t : 0];
}

// the typing of every operation in post order, as the model's sig ("OP
// LFROM>LAS RFROM>RAS PATH RESULT;"); an operation of two constants is
// folded, no operation
void render(const exprtyping_node *n, std::string &out)
{
    if (n->kind == ETN_NEG || n->kind == ETN_NOT)
        render(n->l, out);
    if (n->kind != ETN_BIN)
        return;
    render(n->l, out);
    render(n->r, out);
    if (!n->typed || (n->o.lfrom == ETY_CONST && n->o.rfrom == ETY_CONST))
        return;
    char b[48];
    std::snprintf(b, sizeof b, "%s %c>%c %c>%c %c %c;", opkey(n->op), tlet(n->o.lfrom, 0), tlet(n->o.las, n->o.lwide),
                  tlet(n->o.rfrom, 0), tlet(n->o.ras, n->o.rwide), n->o.path, tlet(n->o.result, 0));
    out += b;
}

// the model's consumer letter of the pass's result under want
int consumer_letter(const exprtyping_result &r, unsigned want)
{
    switch (r.consumer) {
    case ETC_BOOL: return 'c';
    case ETC_KEEP: return want ? 'q' : 'z';
    case ETC_FIT: return 'z';
    case ETC_FLOOR: return 'f';
    case ETC_TRUNC: return 't';
    default: return 0;
    }
}

struct outcome {
    bool refused;
    std::string sig;
    int consumer;
};

outcome mine(exprtyping_node *root, unsigned want, exprtyping_result *res = nullptr)
{
    exprtyping_result r;
    outcome o;

    o.refused = exprtyping_type(root, want, &r) != 0;
    if (!o.refused)
        render(root, o.sig);
    o.consumer = o.refused ? 0 : consumer_letter(r, want);
    if (res)
        *res = r;
    return o;
}

outcome theirs(const struct em_expr *x, enum em_ctx ctx)
{
    em_exprval v;
    outcome o;

    em_expr_eval(x, ctx, DEC, &v);
    o.refused = !std::strncmp(v.v.text, "!noncompile", 11);
    o.sig = o.refused ? "" : v.sig;
    o.consumer = o.refused ? 0 : v.consumer;
    return o;
}

std::string theirs_value(const struct em_expr *x, enum em_ctx ctx)
{
    em_exprval v;

    em_expr_eval(x, ctx, DEC, &v);
    return v.v.text;
}

std::string show(const outcome &o)
{
    if (o.refused)
        return "refused";
    return "[" + o.sig + "] " + (o.consumer ? std::string(1, (char)o.consumer) : std::string("-"));
}

// the differences of one expression in every context (empty: none)
std::vector<std::string> compare(const std::string &text, const std::string &vars)
{
    std::vector<std::string> d;
    tree t(text, vars);
    struct em_expr *x = nullptr;
    char *err = nullptr;

    if (em_expr_parse(text.c_str(), vars.c_str(), &x, &err) || !t.root) {
        d.push_back(text + ": not parsed (" + (err ? err : "") + t.err + ")");
        std::free(err);
        return d;
    }
    for (enum em_ctx ctx : CONTEXTS) {
        outcome a = mine(t.root, want_of(ctx)), b = theirs(x, ctx);

        if (a.refused != b.refused || a.sig != b.sig || a.consumer != b.consumer)
            d.push_back(text + " (" + vars + ") " + em_ctx_name(ctx) + ": the pass " + show(a) + ", the model "
                        + show(b));
    }
    em_expr_free(x);
    return d;
}

std::string readfile(const char *path)
{
    std::ifstream f(path, std::ios::binary);
    std::ostringstream s;

    s << f.rdbuf();
    return s.str();
}

// the text of a cell (its operands as variables of their kind or the
// set's constants, a shift's and ^'s right operand the set's count)
std::string cell_text(const em_matrix &m, const em_cell &c)
{
    const em_set &s = m.sets[c.set];
    auto operand = [&](enum em_kind k, bool count) {
        static const char *const names[] = { "b.b", "w.w", "l.l", "q.q", "f.f" };

        if (k <= EM_F)
            return std::string(names[k]);
        return std::string(count ? s.count[k] : s.value[k]);
    };
    if (c.right < 0)
        return std::string(c.op == EM_NEG ? "-" : "NOT ") + operand(c.left, false);
    bool count = c.op == EM_POW || (c.op >= EM_LSL && c.op <= EM_ASR);
    std::string r = operand((enum em_kind)c.right, count);
    return operand(c.left, false) + " " + em_op_text(c.op) + " " + r;
}

const char *const CELLVARS = "b.b=1 w.w=1 l.l=1 q=1 f.f=1";

// the chains: (text, values of the variables); the values in the Quick
// range where the model types a 48.16 value by its size (c7regf: a Quick
// when it fits)
struct chain {
    const char *text, *vars;
};

const chain PAIRS[][2] = {
    // conflicts 12-15 of the analysis: both orders of the operands
    { { "(l+q+p) AND 255", "l.l=10 q=1.5 p=1.5" }, { "(q+p+l) AND 255", "l.l=10 q=1.5 p=1.5" } },
    { { "(l+q+p) LSL 1", "l.l=10 q=1.5 p=1.5" }, { "(q+p+l) LSL 1", "l.l=10 q=1.5 p=1.5" } },
    { { "NOT (l+q+p)", "l.l=10 q=1.5 p=1.5" }, { "NOT (q+p+l)", "l.l=10 q=1.5 p=1.5" } },
    { { "l+q+f", "l.l=10 q=1.5 f.f=0.5" }, { "f+q+l", "l.l=10 q=1.5 f.f=0.5" } },
    { { "w+v+q", "w.w=20000 v.w=20000 q=0.5" }, { "q+w+v", "w.w=20000 v.w=20000 q=0.5" } },
    { { "w+1+q", "w.w=32767 q=0.5" }, { "q+w+1", "w.w=32767 q=0.5" } },
    { { "q*q2*l", "q=300 q2=300 l.l=2" }, { "l*q*q2", "q=300 q2=300 l.l=2" } },
    { { "base+q*4", "base.l=2097152 q=1.5" }, { "q*4+base", "base.l=2097152 q=1.5" } },
    { { "q+w/2", "q=0.5 w.w=3" }, { "w/2+q", "q=0.5 w.w=3" } },
    { { "w+v+q*2", "q=1.5 w.w=3 v.w=4" }, { "q*2+w+v", "q=1.5 w.w=3 v.w=4" } },
    { { "l+q*2>12", "l.l=10 q=1.5" }, { "q*2+l>12", "l.l=10 q=1.5" } },
    { { "q AND l", "l.l=65536 q=-1" }, { "l AND q", "l.l=65536 q=-1" } },
    { { "b AND 1.5", "b.b=3" }, { "1.5 AND b", "b.b=3" } },
};

const chain CHAINS[] = {
    // the idioms of conflicts 1-4
    { "i/2*2", "i=7" },
    { "q/2+0", "q=-3" },
    { "q/2", "q=-3" },
    { "q MOD 2", "q=-7" },
    // conflicts 5-11 and 16
    { "l*0.1", "l.l=100" },
    { "l*0.001", "l.l=1000000" },
    { "w*0.1", "w.w=100" },
    { "q*1.1", "q=10" },
    { "l*0.5", "l.l=-3" },
    { "1.5-l", "l.l=3" },
    { "l+0.5", "l.l=-3" },
    { "l/1.5", "l.l=-10" },
    { "l*q", "l.l=100000 q=0.25" },
    { "(l+q)*p", "l.l=100 q=0.25 p=2" },
    { "q*40000", "q=1.5" },
    { "l*(a>b)", "l.l=100000 a=2 b=1" },
    { "(a>b)*100000", "a=2 b=1" },
    { "(q LSL 3)+8-w/2", "q=10 w.w=91" },
    { "q ASL 4", "q=1.5" },
    { "x+1>y", "x=1 y=2" },
    { "x+1", "x=1" },
    { "i*300", "i=300" },
    { "q+20000", "q=20000" },
    { "l*1.1", "l.l=10" },
    // #1183's count cases and the spec's scenarios
    { "w+v>c", "w.w=20000 v.w=20000 c.w=0" },
    { "w-(-v)>c", "w.w=20000 v.w=20000 c.w=0" },
    { "q*1000>50000", "q=100" },
    { "l1+1>40000", "l1.l=40000" },
    { "l1/3>3.2", "l1.l=10" },
    { "l1/3.0>3.2", "l1.l=10" },
    { "l1/3.0", "l1.l=10" },
    { "b1+b2>150", "b1.b=100 b2.b=100" },
    { "100000001>l3", "l3.l=100000000" },
    { "q*2>2.4", "q=1.25" },
    { "q+q>2.4", "q=1.25" },
    { "-q<-1.25", "q=1.5" },
    { "1+1+1+q", "q=1.5" },
    { "l+q+p", "l.l=10 q=1.5 p=1.5" },
    { "q+l-l", "l.l=10 q=1.5" },
    { "base<i*4+base", "base.l=100000 i=3" },
    // the four rule points of "expr-tree-5 group 3 findings"
    { "(q+40000) AND b", "q=1.5 b.b=3" },
    { "(l+q) LSL 1", "l.l=100000 q=1.5" },
    { "NOT (l+q)", "l.l=100000 q=1.5" },
    { "(l+q)*f", "l.l=100000 q=1.5 f.f=2" },
    { "b MOD 1.5", "b.b=4" },
    { "f MOD g", "f.f=7.5 g.f=2" },
    { "w MOD 0.5", "w.w=7" },
    { "1.5 AND 3", "" },
    { "(1.5 AND 3)+b", "b.b=1" },
    { "(1.5 AND 3)+0.5", "" },
    { "2.5 OR 1.5", "" },
    { "l*q/l2", "l.l=2000000000 q=2 l2.l=4" },
    // the readings of "expr-tree-5 group 3: three readings of the four rule
    // points": constant comparisons folded, a comparison's result whole
    { "1.5 AND (3=1)", "" },
    { "1.5 AND 3=3", "" },
    { "(3=1)+q", "q=1.5" },
    { "(b>0.5)+(q*2)", "b.b=1 q=1.5" },
    { "(b>0.5)+(q*2)+l", "b.b=1 q=1.5 l.l=100000" },
    { "(f>0.5) AND (l+q)", "f.f=1 q=1.5 l.l=100000" },
    { "q=0.1", "q=0.1" },
    { "q MOD 2", "q=7.5" },
    { "w/2>q", "w.w=3 q=1.2" },
    { "w/2*2<>x", "w.w=3 x=3" },
    { "w/2*2=q", "w.w=3 q=2" },
    { "w/2*2=w", "w.w=3" },
    // fixed point 3: Quick as a whole number
    { "Not h", "h=-1" },
    { "(q AND 1)=1", "q=3" },
    { "h AND g", "h=-1 g=-1" },
    { "flag=(a>b)", "flag=-1 a=2 b=1" },
    { "(flag=-1)*5", "flag=-1" },
    { "x-(a>b)", "x=10 a=2 b=1" },
    { "x*(y>0)+1", "x=3 y=1" },
    { "(x AND 3)=q", "x=7 q=3" },
    { "q MOD 2=1", "q=7" },
    // constants with a point, bit operators, shifts
    { "b/2.0", "b.b=3" },
    { "q*3.0", "q=1.5" },
    { "q+3.0", "q=1.5" },
    { "b AND 2.0", "b.b=3" },
    { "b AND -1.5", "b.b=7" },
    { "NOT 1.5", "" },
    { "w OR 2.5", "w.w=4" },
    { "2.5 EOR l", "l.l=7" },
    { "b LSL 2.0", "b.b=3" },
    { "q LSL 2.5", "q=1.5" },
    { "b LSL (1+1.0)", "b.b=3" },
    { "x LSL qh", "x.w=3 qh=2.5" },
    { "l LSR qh", "l.l=100000 qh=2.5" },
    { "x LSL (qh*2)", "x.w=3 qh=2.5" },
    { "1.5 LSL 2", "" },
    { "1.5 LSR 1", "" },
    { "-1.5 ASR 1", "" },
    { "(1.5 LSL 2)+1", "" },
    { "1.5 LSL qh", "qh=2.5" },
    // a bit operator in a comparison with a Float
    { "(b AND 1)>f", "b.b=3 f.f=0.5" },
    { "(b AND 1)=2.5", "b.b=3" },
    { "2.0=(b AND 1)", "b.b=3" },
    { "(l LSL 1)>2.5", "l.l=3" },
    { "(NOT b)<1.5", "b.b=3" },
    { "(b AND 1)+0.5>1", "b.b=3" },
    { "(b>0.5)+q", "b.b=1 q=1.5" },
    // division by 0 in the wanted type's format, Mod by 0
    { "l/q", "l.l=100 q=0" },
    { "(l+q)/p", "l.l=100 q=0.5 p=0" },
    { "l MOD q", "l.l=100000 q=0" },
    { "q MOD p", "q=7.5 p=0.5" },
    // Float beside a Quick, ^
    { "q*f+l", "q=1.5 f.f=2 l.l=3" },
    { "(q+l)*f", "q=1.5 f.f=2 l.l=3" },
    { "q^2+l", "q=1.5 l.l=3" },
    { "(q*2+l)+f", "q=1.5 f.f=2 l.l=3" },
};

} // namespace

TEST_CASE("exprtyping: every cell of the matrix as the model types it (P+)", "[exprtyping]")
{
    std::string text = readfile(EXPRMATRIX_TXT);
    em_matrix m{};
    char *errors = nullptr;
    size_t compared = 0, refused = 0, wide = 0;
    std::vector<std::string> d;

    REQUIRE(!text.empty());
    REQUIRE(em_parse(text.data(), text.size(), EXPRMATRIX_TXT, &m, &errors) == 0);
    for (size_t i = 0; i < m.ncells; i++) {
        const em_cell &c = m.cells[i];
        struct em_expr *x = nullptr;

        if (c.ctx == EM_I1 || c.ctx == EM_I2 || em_cell_expr(&m, &c, &x))
            continue;
        tree t(cell_text(m, c), CELLVARS);
        if (!t.root) {
            d.push_back(std::string(c.key) + ": not parsed: " + t.err);
            em_expr_free(x);
            continue;
        }
        outcome a = mine(t.root, want_of(c.ctx)), b = theirs(x, c.ctx);
        em_expr_free(x);
        compared++;
        refused += b.refused;
        wide += b.sig.find(" 3 ") != std::string::npos;
        if (a.refused != b.refused || a.sig != b.sig || a.consumer != b.consumer)
            d.push_back(std::string(c.key) + " " + cell_text(m, c) + ": the pass " + show(a) + ", the model " + show(b));
    }
    for (size_t i = 0; i < d.size() && i < 25; i++)
        FAIL_CHECK(d[i]);
    CHECK(d.empty());
    CHECK(compared > 30000);
    CHECK(refused > 1000);
    CHECK(wide > 100);
    em_free(&m);
}

TEST_CASE("exprtyping: the chains of the analysis as the model types them", "[exprtyping]")
{
    std::vector<std::string> d;

    for (const auto &c : CHAINS)
        for (const auto &s : compare(c.text, c.vars))
            d.push_back(s);
    for (const auto &p : PAIRS)
        for (const auto &c : p)
            for (const auto &s : compare(c.text, c.vars))
                d.push_back(s);
    for (size_t i = 0; i < d.size() && i < 25; i++)
        FAIL_CHECK(d[i]);
    CHECK(d.empty());
}

TEST_CASE("exprtyping: the order pairs are typed alike (fixed point 1)", "[exprtyping]")
{
    // the expression's type, format and consumer and the path of its last
    // operation are the same in both orders of the operands (the
    // operations below it are other ones: (l+q)+f is no (f+q)+l)
    for (const auto &p : PAIRS)
        for (enum em_ctx ctx : CONTEXTS) {
            tree a(p[0].text, p[0].vars), b(p[1].text, p[1].vars);
            exprtyping_result ra, rb;
            REQUIRE(a.root);
            REQUIRE(b.root);
            outcome oa = mine(a.root, want_of(ctx), &ra), ob = mine(b.root, want_of(ctx), &rb);

            INFO(p[0].text, " / ", p[1].text, " ", em_ctx_name(ctx));
            CHECK(oa.refused == ob.refused);
            if (oa.refused || ob.refused)
                continue;
            CHECK(ra.fmt == rb.fmt);
            CHECK(ra.consumer == rb.consumer);
            CHECK(ra.t == rb.t);
            CHECK(a.root->o.path == b.root->o.path);
            CHECK(a.root->t == b.root->t);
        }
}

TEST_CASE("exprtyping: the rounding point and the formats of the analysis", "[exprtyping]")
{
    struct {
        const char *text;
        unsigned want;
        unsigned consumer, fmt;
    } cases[] = {
        { "i/2*2", ET_LONG, ETC_FLOOR, ETF_Q48 },  // a / under a wanted Long whole at once
        { "q/2", ET_LONG, ETC_TRUNC, ETF_Q48 },    // a / at the top towards zero
        { "q/2", ET_WORD, ETC_FLOOR, ETF_Q16 },    // a Word destination rounded down
        { "q*q2*l", ET_NONE, ETC_FIT, ETF_Q48 },   // Print: a Quick when it fits
        { "q+w/2", ET_NONE, ETC_KEEP, ETF_Q16 },   // 16.16 without a Long
        { "l*q", ET_WORD, ETC_FLOOR, ETF_Q48 },    // a Long under a Word computed wide
        { "q*40000", ET_QUICK, ETC_KEEP, ETF_Q48 }, // a Long constant: 48.16, wrapped to the Quick as today
        { "l+q*2>12", ET_NONE, ETC_BOOL, ETF_TYPE },
    };

    for (const auto &c : cases) {
        tree t(c.text, "i=7 q=1.5 q2=2 l.l=3 w.w=3");
        exprtyping_result r;

        INFO(c.text);
        REQUIRE(t.root);
        REQUIRE(exprtyping_type(t.root, c.want, &r) == 0);
        CHECK(r.consumer == c.consumer);
        CHECK(r.fmt == c.fmt);
    }
}

TEST_CASE("exprtyping: the fast path of a 48.16 operation", "[exprtyping]")
{
    // an operation is on the fast path when both operands are narrow: the
    // largest values of their formats are narrow by exprarith's test
    auto widest = [](const exprtyping_node *n) -> int64_t {
        if (exprtyping_isconst(n)) {
            // the folded values of these tests are whole constants or n.5
            const exprtyping_node *k = n->kind == ETN_NEG ? n->l : n;
            int64_t v = k->num * 65536 / k->den;
            return n->kind == ETN_NEG ? -v : v;
        }
        if (n->fmt == ETF_Q48)
            return exprarith_whole_q48(INT32_MAX);
        if (n->fmt == ETF_Q16 || n->t == ET_QUICK)
            return exprarith_q16_q48(INT32_MIN);
        switch (n->t) {
        case ET_BYTE: return exprarith_whole_q48(-128);
        case ET_WORD: return exprarith_whole_q48(-32768);
        default: return exprarith_whole_q48(INT32_MAX);
        }
    };
    size_t fast = 0, slow = 0;
    static const chain cases[] = {
        { "l*q", "l.l=3 q=1.5" },     { "w*q", "w.w=3 q=1.5" },       { "l+q+p", "l.l=3 q=1.5 p=1" },
        { "q*q2*l", "q=1 q2=1 l.l=1" }, { "b+q", "b.b=1 q=1" },        { "q*40000", "q=1" },
        { "(a>b)*q", "a=1 b=2 q=1" },  { "q/2", "q=1" },               { "100000+q", "q=1" },
    };

    for (const auto &c : cases)
        for (unsigned want : { ET_LONG, ET_WORD, ET_QUICK, ET_NONE }) {
            tree t(c.text, c.vars);
            exprtyping_result r;

            REQUIRE(t.root);
            if (exprtyping_type(t.root, want, &r))
                continue;
            for (const auto &n : t.nodes) {
                if (n.kind != ETN_BIN || !n.typed || n.o.path != ETY_Q48)
                    continue;
                bool narrow = exprarith_narrow(widest(n.l)) && exprarith_narrow(widest(n.r));

                INFO(c.text, " ", want);
                CHECK((n.o.fast != 0) == narrow);
                (n.o.fast ? fast : slow)++;
            }
        }
    CHECK(fast > 0);
    CHECK(slow > 0);
}

TEST_CASE("exprtyping: every chain of two operators as the model types it", "[exprtyping]")
{
    // a OP1 b OP2 c and a OP1 (b OP2 c) for the operators and operand
    // kinds below, in every context, with values beyond a Word and a Quick
    // too (a 48.16 subtree is taken by the operation, not by its value)
    static const char *const ops[] = { "+", "-", "*", "/", " MOD ", " AND ", " LSL ", "=", "<", "^" };
    static const char *const kinds[] = { "b", "w", "l", "q", "f", "-q", "3", "1000", "40000", "1.5", "-1.5", "0.1" };
    const char *vars = "b.b=3 w.w=30000 l.l=100000 q=1.5 f.f=2.5";
    std::vector<std::string> d;
    size_t n = 0;

    for (const char *o1 : ops)
        for (const char *o2 : ops)
            for (const char *a : kinds)
                for (const char *b : kinds)
                    for (const char *c : kinds)
                        for (int br = 0; br < 2; br++) {
                            std::string t = br ? std::string(a) + o1 + "(" + b + o2 + c + ")"
                                               : std::string(a) + o1 + b + o2 + c;
                            for (const auto &s : compare(t, vars))
                                d.push_back(s);
                            n++;
                        }
    for (size_t i = 0; i < d.size() && i < 40; i++)
        FAIL_CHECK(d[i]);
    INFO(d.size(), " of ", n * (sizeof CONTEXTS / sizeof CONTEXTS[0]));
    CHECK(d.empty());
}

TEST_CASE("exprtyping: constant subtrees fold as the model folds them", "[exprtyping]")
{
    // the value of a constant subtree (exprtyping_constvalue) against the
    // model's value under a wanted Long, where it is a whole number of a
    // Long: bit operators with fractions and comparisons folded by the rule
    // ("expr-tree-5 group 3: three readings of the four rule points"); a
    // subtree without a fraction under a bit operator the model folds by
    // today's flow (task 3.6 keeps that fold byte-identical), not compared
    std::function<bool(const exprtyping_node *)> hasfrac = [&](const exprtyping_node *n) -> bool {
        int64_t num, den;

        if (n->kind == ETN_CONST && n->point)
            return true;
        if (n->kind == ETN_NEG)
            return hasfrac(n->l);
        if (n->kind == ETN_BIN && (hasfrac(n->l) || hasfrac(n->r)))
            return true;
        return !exprtyping_constvalue(n, &num, &den) && den != 1;
    };
    std::function<bool(const exprtyping_node *)> bitfraction = [&](const exprtyping_node *n) -> bool {
        if (n->kind == ETN_NEG)
            return bitfraction(n->l);
        if (n->kind != ETN_BIN)
            return false;
        if ((n->op == ETO_AND || n->op == ETO_OR || n->op == ETO_EOR) && (hasfrac(n->l) || hasfrac(n->r)))
            return true;
        return bitfraction(n->l) || bitfraction(n->r);
    };
    static const char *const ops[] = { " AND ", " OR ", " EOR ", "=", "<", ">", "+", "-", "*" };
    static const char *const k[] = { "3", "1.5", "0", "-2", "2.5", "40000" };
    std::vector<std::string> d;
    size_t compared = 0;

    for (const char *o1 : ops)
        for (const char *o2 : ops)
            for (const char *a : k)
                for (const char *b : k)
                    for (const char *c : k)
                        for (int br = 0; br < 2; br++) {
                            std::string t = br ? std::string(a) + o1 + "(" + b + o2 + c + ")"
                                               : std::string(a) + o1 + b + o2 + c;
                            tree tr(t, "");
                            struct em_expr *x = nullptr;
                            char *err = nullptr;
                            int64_t num, den;

                            REQUIRE(tr.root);
                            REQUIRE(em_expr_parse(t.c_str(), "", &x, &err) == 0);
                            std::string m = theirs_value(x, EM_DL);
                            em_expr_free(x);
                            if (!bitfraction(tr.root) || exprtyping_constvalue(tr.root, &num, &den) || den != 1
                                || num < INT32_MIN || num > INT32_MAX || m[0] == '!')
                                continue;
                            compared++;
                            if (m != std::to_string(num))
                                d.push_back(t + ": the pass " + std::to_string(num) + ", the model " + m);
                        }
    for (size_t i = 0; i < d.size() && i < 25; i++)
        FAIL_CHECK(d[i]);
    CHECK(d.empty());
    CHECK(compared > 10000);
}
