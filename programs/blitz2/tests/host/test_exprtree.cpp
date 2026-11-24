// test_exprtree - the nodes of an expression and the walk over them
// (programs/blitz2/src/exprtree.c; issue #1180).  The hooks of the walk
// are a small evaluator of
// this file over a text: a letter is an operand (a simple element, whose
// leaf is made, emitted and released as in eval.c), a capital letter with
// [ ] an array element (its
// index, a step of its path and its variable node as in parse.c's
// varparse), a capital letter with { } a call
// (its node live while its parameters, separated by commas, are read, as
// eval.c's afunction and amigalib), + -
// binary operators of one precedence, * / of a higher one, ^ of a higher
// one still, ( ) a
// bracket and ~ NOT (each an expression node of its own, as eval4 makes
// them), _ a minus sign (a run of them is one node, as eval.c's negate
// makes it).  Each emitted operator is appended to the output, so the
// output is the expression in postfix order.  Tested: the order of the
// emission, the counted nodes' limit (the 65th is exprtree_overflow), the
// bound of the arena (EXPRTREE_NODES is never reached), the reset, and
// that the walk's stack does not grow along a chain of one precedence
// (100 000 operators).
#include <csetjmp>
#include <cstddef>
#include <cstdint>
#include <string>
#include <snitch/snitch.hpp>
extern "C" {
#include "exprtree.h"
}

namespace {

enum { OP_NONE = 0xffff };

struct ctx {
    const char *p;              // the text not read yet
    std::string out;            // the postfix output
    std::uintptr_t lo, hi;      // the lowest and highest stack address in exprtree_operand
    unsigned maxlive, maxpend;  // the arena's high water
};

std::jmp_buf errjmp;            // exprtree_overflow (1), exprtree_full (2), a syntax error (3)

unsigned precof(char c)
{
    switch (c) {
    case '+': case '-': return 10;
    case '*': case '/': return 20;
    case '^': return 30;
    default: return 0;
    }
}

void note(ctx *x)
{
    if (exprtree_live() > x->maxlive)
        x->maxlive = exprtree_live();
    if (exprtree_pending() > x->maxpend)
        x->maxpend = exprtree_pending();
}

void element(ctx *x);

// eval4: an expression node of kind kind, the walk, the node left pending
void expression(ctx *x, unsigned kind)
{
    unsigned op;

    exprtree_push(exprtree_new(kind), OP_NONE, 1);
    note(x);
    if (exprtree_climb(x, &op))
        std::longjmp(errjmp, 3);
}

// getelement2: a letter, a bracket, NOT or a run of minus signs
void element(ctx *x)
{
    char c = *x->p++;

    if (c == '(') {
        expression(x, EXN_BRACKET);
        exprtree_release(exprtree_top());
        if (*x->p++ != ')')
            std::longjmp(errjmp, 3);
    } else if (c == '~') {
        expression(x, EXN_NOT);
        x->out += '~';
        exprtree_release(exprtree_top());
    } else if (c >= 'a' && c <= 'z') {
        // a simple element: its leaf, emitted at once and released (eval.c:
        // newleaf ... leafdone)
        exprnode *l = exprtree_new(EXN_LEAF);

        l->op = ELK_VAR;
        note(x);
        x->out += c;
        exprtree_release(l);
    } else if (c >= 'A' && c <= 'Z' && *x->p == '{') {
        // a call (eval.c: afunction, amigalib, libfunction, all of one
        // shape): its node lives while its parameters are read, each an
        // expression of its own, and is then the leaf of its value
        // (callcont)
        exprnode *n = exprtree_new(EXN_CALL);

        n->op = CLK_PROC;
        x->p++;
        note(x);
        if (*x->p != '}')
            for (;;) {
                expression(x, EXN_EXPR);
                exprtree_release(exprtree_top());
                if (*x->p != ',')
                    break;
                x->p++;
            }
        if (*x->p++ != '}')
            std::longjmp(errjmp, 3);
        n->kind = EXN_LEAF;
        n->op = 0;
        x->out += c;
        exprtree_release(n);
    } else if (c >= 'A' && c <= 'Z') {
        // an array element (parse.c, varparse): its index an expression of
        // its own, then a step of its path, emitted and released, then the
        // variable node, which becomes the element's leaf (eval.c:
        // variable, varleaf)
        exprnode *s, *l;

        if (*x->p++ != '[')
            std::longjmp(errjmp, 3);
        expression(x, EXN_EXPR);
        exprtree_release(exprtree_top());
        if (*x->p++ != ']')
            std::longjmp(errjmp, 3);
        s = exprtree_new(EXN_STEP);
        s->op = VST_INDEX;
        note(x);
        exprtree_release(s);
        l = exprtree_new(EXN_VAR);
        note(x);
        l->kind = EXN_LEAF;
        l->op = ELK_VAR;
        x->out += c;
        exprtree_release(l);
    } else
        std::longjmp(errjmp, 3);
}

} // namespace

extern "C" {

unsigned exprtree_operand(void *v, unsigned *op)
{
    ctx *x = static_cast<ctx *>(v);
    auto at = reinterpret_cast<std::uintptr_t>(__builtin_frame_address(0));
    unsigned prec;

    if (at < x->lo)
        x->lo = at;
    if (at > x->hi)
        x->hi = at;
    if (*x->p == '_') {             // a run of minus signs: one node
        exprnode *n = exprtree_new(EXN_NEG);
        unsigned k = 0;

        note(x);
        while (*x->p == '_') {
            x->p++;
            k++;
        }
        n->op = static_cast<unsigned short>(k);
        element(x);
        x->out.append(k, '_');
        exprtree_release(n);
    } else
        element(x);
    prec = precof(*x->p);
    if (prec) {
        *op = static_cast<unsigned char>(*x->p);
        x->p++;
    } else
        *op = OP_NONE;
    return prec;
}

void exprtree_left(void *v, exprnode *l, unsigned)
{
    note(static_cast<ctx *>(v));
    l->end = 1;
}

void exprtree_done(void *, exprnode *) {}

void exprtree_pre(void *, exprnode *) {}

void exprtree_enter(void *v, exprnode *)
{
    note(static_cast<ctx *>(v));
}

void exprtree_emit(void *v, exprnode *n)
{
    static_cast<ctx *>(v)->out += static_cast<char>(n->op);
}

void exprtree_overflow(void)
{
    std::longjmp(errjmp, 1);
}

void exprtree_full(void)
{
    std::longjmp(errjmp, 2);
}

} // extern "C"

namespace {

// the expression text in postfix order, or "overflow", "full", "syntax";
// x gets the high water and the stack addresses
std::string walk(const std::string &text, ctx &x)
{
    x.p = text.c_str();
    x.out.clear();
    x.lo = UINTPTR_MAX;
    x.hi = 0;
    x.maxlive = x.maxpend = 0;
    exprtree_reset();
    switch (setjmp(errjmp)) {
    case 0:
        break;
    case 1:
        return "overflow";
    case 2:
        return "full";
    default:
        return "syntax";
    }
    expression(&x, EXN_EXPR);
    exprtree_release(exprtree_top());
    if (*x.p)
        return "syntax";
    return x.out;
}

std::string walk(const std::string &text)
{
    ctx x;

    return walk(text, x);
}

std::string times(const char *s, unsigned n)
{
    std::string r;

    while (n--)
        r += s;
    return r;
}

} // namespace

TEST_CASE("exprtree: the order of the emission", "[exprtree]")
{
    CHECK(walk("a") == "a");
    CHECK(walk("a+b") == "ab+");
    CHECK(walk("a+b*c") == "abc*+");
    CHECK(walk("a*b+c") == "ab*c+");
    CHECK(walk("a+b+c") == "ab+c+");        // one precedence: from the left
    CHECK(walk("a-b+c-d") == "ab-c+d-");
    CHECK(walk("a^b^c") == "ab^c^");
    CHECK(walk("a+b*c^d-e") == "abcd^*+e-");
    CHECK(walk("(a+b)*c") == "ab+c*");
    CHECK(walk("a*(b+c)") == "abc+*");
    CHECK(walk("~a+b") == "ab+~");          // NOT takes the rest of its expression
    CHECK(walk("a+~b*c") == "abc*~+");
    CHECK(walk("_a+b") == "a_b+");
    CHECK(walk("a*__b") == "ab__*");
    CHECK(walk("_(a+b)") == "ab+_");
    CHECK(walk("A[b+c]*d") == "bc+Ad*");   // the index before the array
    CHECK(walk("a+A[B[c]]") == "acBA+");
    CHECK(walk("F{a+b}*c") == "ab+Fc*");   // the parameters before the call
    CHECK(walk("a+F{}") == "aF+");
    CHECK(walk("F{a,G{b*c},d}") == "abc*GdF");
    CHECK(walk("F{a") == "syntax");
    CHECK(walk("a+") == "syntax");
    CHECK(walk("(a") == "syntax");
}

TEST_CASE("exprtree: the arena is empty after an expression and after a reset", "[exprtree]")
{
    ctx x;

    CHECK(walk("a+(b*c)-~d", x) == "abc*+d~-");
    CHECK(exprtree_live() == 0);
    CHECK(exprtree_pending() == 0);
    CHECK(x.maxpend == 4);                 // the expression, +, the bracket, *
    exprtree_reset();
    exprtree_push(exprtree_new(EXN_EXPR), OP_NONE, 1);
    exprtree_new(EXN_LEAF);
    CHECK(exprtree_live() == 2);
    CHECK(exprtree_pending() == 1);
    CHECK(exprtree_top() != nullptr);
    exprtree_reset();
    CHECK(exprtree_live() == 0);
    CHECK(exprtree_pending() == 0);
    CHECK(exprtree_top() == nullptr);
}

TEST_CASE("exprtree: 64 counted nodes, the 65th is the overflow", "[exprtree]")
{
    ctx x;

    // the expression and 63 brackets
    CHECK(walk(times("(", 63) + "a" + times(")", 63), x) == "a");
    CHECK(x.maxpend == 64);
    CHECK(walk(times("(", 64) + "a" + times(")", 64)) == "overflow");
    // NOT, brackets and pending operators count alike
    CHECK(walk("a+(b*~" + times("(", 59) + "c" + times(")", 60), x) == "abc~*+");
    CHECK(x.maxpend == 64);
    CHECK(walk("a+(b*~" + times("(", 60) + "c" + times(")", 61)) == "overflow");
    // a minus sign is not counted, but each is a node in the arena: the
    // bound of EXPRTREE_NODES
    CHECK(walk(times("_(", 63) + "a" + times(")", 63), x) == "a" + times("_", 63));
    CHECK(x.maxpend == 64);
    CHECK(x.maxlive == 128);
    CHECK(walk(times("_(", 64) + "a" + times(")", 64)) == "overflow");
    // an element's leaf behind the run of minus signs of the innermost
    // level: the other way to the bound (129)
    CHECK(walk(times("_(", 63) + "_a" + times(")", 63), x) == "a" + times("_", 64));
    CHECK(x.maxlive == 129);
    CHECK(x.maxlive <= EXPRTREE_NODES);
    // arrays nested in their indexes behind minus signs: each index is counted, a step and the
    // variable node live only at the leaf's place: the bound stays 129
    CHECK(walk(times("_A[", 63) + "_b" + times("]", 63), x) == "b_" + times("A_", 63));
    CHECK(x.maxpend == 64);
    CHECK(x.maxlive == 129);
    CHECK(walk(times("_A[", 64) + "b" + times("]", 64)) == "overflow");
    // calls nested in their parameters behind minus signs: a call's node lives beside the run of minus
    // signs of its level while its parameter is read.  The compiler nests
    // at most 32 calls (each holds an entry of forthstack, the 33rd is
    // "Expression too Complex"), so the bound is 64 counted nodes, the
    // 65th, 64 runs of minus signs and 32 calls: 161
    CHECK(walk(times("_F{", 31) + times("_(", 32) + "_b" + times(")", 32) + times("}", 31), x)
          == "b" + times("_", 33) + times("F_", 31));
    CHECK(x.maxpend == 64);
    CHECK(x.maxlive == 160);
    CHECK(walk(times("_F{", 31) + times("_(", 32) + "_G{}" + times(")", 32) + times("}", 31), x)
          == "G" + times("_", 33) + times("F_", 31));
    CHECK(x.maxlive == 160);
    CHECK(walk(times("_F{", 32) + times("_(", 32) + "b" + times(")", 32) + times("}", 32))
          == "overflow");
    CHECK(161 <= EXPRTREE_NODES);
}

TEST_CASE("exprtree: a chain of one precedence does not grow the stack", "[exprtree]")
{
    ctx small, big;
    std::string a = "a" + times("+a", 10);
    std::string b = "a" + times("+a", 100000);

    CHECK(walk(a, small).size() == 21);
    CHECK(walk(b, big).size() == 200001);
    CHECK(big.maxlive == small.maxlive);
    CHECK(big.maxlive <= 4);
    CHECK(big.hi - big.lo == small.hi - small.lo);
    // a higher precedence is one level deeper each, as the original's
    // recursion: a*a+a*a+... stays two levels
    ctx mixed;
    CHECK(walk("a" + times("*a+a", 50000), mixed).size() == 200001);
    CHECK(mixed.maxpend <= 3);
}
