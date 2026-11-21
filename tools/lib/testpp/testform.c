/*
 * testform.c - the form of a Blitz test source (testpp.h, tp_check_form).
 */
#include "testpp.h"
#include "testpp_int.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

/* the control structures that must not cross a block */
enum { C_FOR, C_WHILE, C_REPEAT, C_SELECT, C_IF, C_SETERR };
static const char *const ctl_names[] = { "For", "While", "Repeat", "Select", "If", "SetErr" };

struct ctl {
    int kind;
    unsigned line;
};

struct fstate {
    const struct tp_params *par;
    struct tp_source *src;
    unsigned line;
    int oom;
    int in_macro, in_proc, in_newtype;
    char *macro_name;
    int block;                  /* TP_B_NONE, TP_B_INIT, TP_B_CASE, TP_B_FINAL */
    unsigned block_line;
    char *case_name;
    unsigned init_line, final_line, ncases;
    /* conditional compilation: a frame id per CNIF (a CELSE gives a new one) */
    unsigned *cn, *cn_open;
    size_t ncn, ncn_open, capcn;
    unsigned next_id;
    struct ctl *ctl;
    size_t nctl, capctl;
    char **names;
    unsigned *name_lines;
    size_t nnames;
};

static void ferr(struct fstate *f, unsigned line, const char *fmt, ...) __attribute__((format(printf, 3, 4)));
static void ferr(struct fstate *f, unsigned line, const char *fmt, ...)
{
    char buf[500];
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    if (tp_error(&f->src->errors, f->par->file, line, "%s", buf))
        f->oom = 1;
}

static const char *block_text(int b)
{
    return b == TP_B_INIT ? "the batch init" : b == TP_B_FINAL ? "the batch end" : "the case";
}

static const char *closing_name(int b)
{
    return b == TP_B_INIT ? "!EndTestBatchInit" : b == TP_B_FINAL ? "!EndTestBatchEnd" : "!EndTestCase";
}

/* "the case NAME (line N)" / "the batch init (line N)" of the open block */
static void open_text(const struct fstate *f, char *out, size_t size)
{
    if (f->block == TP_B_CASE)
        snprintf(out, size, "the case %s (line %u)", f->case_name ? f->case_name : "?", f->block_line);
    else
        snprintf(out, size, "%s (line %u)", block_text(f->block), f->block_line);
}

static int is_ident(int c)
{
    return isalnum(c) || c == '_';
}

/* ------------------------------------------------------------ the case names */

static void check_name(struct fstate *f, const char *name, size_t n)
{
    char *s = tp_strndup(name, n);
    int ok = n > 0 && name[0] >= 'a' && name[0] <= 'z';

    if (!s) {
        f->oom = 1;
        return;
    }
    for (size_t i = 0; ok && i < n; i++)
        ok = (name[i] >= 'a' && name[i] <= 'z') || (name[i] >= '0' && name[i] <= '9') || name[i] == '_';
    if (!n)
        ferr(f, f->line, "!TestCase needs the name of the case in braces");
    else if (!ok)
        ferr(f, f->line, "the case name \"%s\" does not match [a-z][a-z0-9_]*", s);
    else if (strstr(s, "__"))
        ferr(f, f->line, "the case name %s holds \"__\" (the driver's labels)", s);
    else if (n >= 10 && !strcmp(s + n - 10, "_originals"))
        ferr(f, f->line, "the case name %s ends in _originals (the name of a registered test's side)", s);
    else {
        for (size_t i = 0; i < f->par->ntokens; i++)
            if (!strcasecmp(f->par->tokens[i], s)) {
                ferr(f, f->line, "the case name %s is the token %s", s, f->par->tokens[i]);
                goto done;
            }
        for (size_t i = 0; i < f->nnames; i++)
            if (!strcmp(f->names[i], s)) {
                ferr(f, f->line, "the case name %s is used twice (line %u)", s, f->name_lines[i]);
                goto done;
            }
    }
    {
        char **v = realloc(f->names, (f->nnames + 1) * sizeof *v);
        unsigned *l = v ? realloc(f->name_lines, (f->nnames + 1) * sizeof *l) : NULL;

        if (v)
            f->names = v;
        if (!v || !l) {
            f->oom = 1;
            free(s);
            return;
        }
        f->name_lines = l;
        f->names[f->nnames] = s;
        f->name_lines[f->nnames++] = f->line;
        return;
    }
done:
    free(s);
}

/* ------------------------------------------------------------ blocks */

static int same_cn(const struct fstate *f)
{
    return f->ncn == f->ncn_open && (!f->ncn || !memcmp(f->cn, f->cn_open, f->ncn * sizeof *f->cn));
}

static void close_block(struct fstate *f, int complete)
{
    char what[200];

    open_text(f, what, sizeof what);
    if (complete) {
        if (!same_cn(f))
            ferr(f, f->line, "conditional compilation cuts %s: a CNIF, CSIF, CELSE or CEND inside it has "
                 "no pair inside it", what);
        if (f->nctl)
            ferr(f, f->line, "%s ends inside the %s of line %u", what, ctl_names[f->ctl[f->nctl - 1].kind],
                 f->ctl[f->nctl - 1].line);
    }
    f->block = TP_B_NONE;
    f->nctl = 0;
    free(f->case_name);
    f->case_name = NULL;
}

static void open_block(struct fstate *f, int b, const char *name, size_t nlen)
{
    char what[200];

    if (f->block != TP_B_NONE) {
        open_text(f, what, sizeof what);
        ferr(f, f->line, "%s has no %s", what, closing_name(f->block));
        close_block(f, 0);
    }
    if (b == TP_B_INIT) {
        if (f->init_line)
            ferr(f, f->line, "a second batch init (the first in line %u)", f->init_line);
        else if (f->ncases)
            ferr(f, f->line, "the batch init must stand before the first case");
        if (!f->init_line)
            f->init_line = f->line;
    } else if (b == TP_B_FINAL) {
        if (f->final_line)
            ferr(f, f->line, "a second batch end (the first in line %u)", f->final_line);
        if (!f->final_line)
            f->final_line = f->line;
    } else {
        if (f->final_line)
            ferr(f, f->line, "a case after the batch end (line %u)", f->final_line);
        check_name(f, name, nlen);
        f->ncases++;
        f->case_name = tp_strndup(name, nlen);
    }
    f->block = b;
    f->block_line = f->line;
    free(f->cn_open);
    f->cn_open = malloc((f->ncn + 1) * sizeof *f->cn_open);
    if (!f->cn_open) {
        f->oom = 1;
        return;
    }
    if (f->ncn)
        memcpy(f->cn_open, f->cn, f->ncn * sizeof *f->cn);
    f->ncn_open = f->ncn;
}

static void end_block(struct fstate *f, int b)
{
    int opens = b == TP_B_END_INIT ? TP_B_INIT : b == TP_B_END_FINAL ? TP_B_FINAL : TP_B_CASE;
    const char *name = closing_name(opens);
    char what[200];

    if (f->block == TP_B_NONE) {
        ferr(f, f->line, "%s without %s before it", name,
             opens == TP_B_CASE ? "a case" : opens == TP_B_INIT ? "a batch init" : "a batch end");
        return;
    }
    if (f->block != opens) {
        open_text(f, what, sizeof what);
        ferr(f, f->line, "%s closes %s", name, what);
        close_block(f, 0);
        return;
    }
    close_block(f, 1);
}

/* ------------------------------------------------------------ statements */

static void push_cn(struct fstate *f)
{
    if (f->ncn == f->capcn) {
        unsigned *v = realloc(f->cn, (f->capcn = f->capcn ? 2 * f->capcn : 8) * sizeof *v);

        if (!v) {
            f->oom = 1;
            return;
        }
        f->cn = v;
    }
    f->cn[f->ncn++] = ++f->next_id;
}

static void push_ctl(struct fstate *f, int kind)
{
    if (f->nctl == f->capctl) {
        struct ctl *v = realloc(f->ctl, (f->capctl = f->capctl ? 2 * f->capctl : 8) * sizeof *v);

        if (!v) {
            f->oom = 1;
            return;
        }
        f->ctl = v;
    }
    f->ctl[f->nctl].kind = kind;
    f->ctl[f->nctl++].line = f->line;
}

static void pop_ctl(struct fstate *f, const char *closer)
{
    char what[200];

    if (f->nctl) {
        f->nctl--;
        return;
    }
    open_text(f, what, sizeof what);
    ferr(f, f->line, "the %s closes a structure opened outside %s", closer, what);
}

/* the offset behind the word Then of l[s..e) outside strings, 0 none */
static size_t then_at(const char *l, size_t s, size_t e)
{
    for (size_t p = s; p < e; p++) {
        if (l[p] == '"') {
            for (p++; p < e && l[p] != '"'; p++)
                ;
            continue;
        }
        if ((p == s || !is_ident((unsigned char)l[p - 1])) && tp_word(l, p, e, "Then", NULL))
            return p + 4;
    }
    return 0;
}

/* the control structures of a statement inside a block */
static void control(struct fstate *f, const char *l, size_t s, size_t e)
{
    size_t p;

    if (tp_word(l, s, e, "For", NULL))
        push_ctl(f, C_FOR);
    else if (tp_word(l, s, e, "While", NULL))
        push_ctl(f, C_WHILE);
    else if (tp_word(l, s, e, "Repeat", NULL))
        push_ctl(f, C_REPEAT);
    else if (tp_word(l, s, e, "Select", NULL))
        push_ctl(f, C_SELECT);
    else if (tp_word(l, s, e, "SetErr", NULL))
        push_ctl(f, C_SETERR);
    else if (tp_word(l, s, e, "Next", NULL))
        pop_ctl(f, "Next");
    else if (tp_word(l, s, e, "Wend", NULL))
        pop_ctl(f, "Wend");
    else if (tp_word(l, s, e, "Until", NULL))
        pop_ctl(f, "Until");
    else if (tp_word(l, s, e, "Forever", NULL))
        pop_ctl(f, "Forever");
    else if (tp_word(l, s, e, "EndIf", NULL))
        pop_ctl(f, "EndIf");
    else if (tp_word(l, s, e, "End", &p)) {
        p = tp_blanks(l, p, e);
        if (tp_word(l, p, e, "If", NULL))
            pop_ctl(f, "End If");
        else if (tp_word(l, p, e, "Select", NULL))
            pop_ctl(f, "End Select");
        else if (tp_word(l, p, e, "SetErr", NULL))
            pop_ctl(f, "End SetErr");
    } else if (tp_word(l, s, e, "If", NULL)) {
        size_t t = then_at(l, s, e);

        if (!t)
            push_ctl(f, C_IF);
        else if ((t = tp_blanks(l, t, e)) < e)
            control(f, l, t, e);        /* If c Then For ... */
    }
}

/* a declaration that may stand outside the blocks */
static int declaration(const char *l, size_t s, size_t e)
{
    static const char *const words[] = { "DEFTYPE", "CERR", "OPTIMIZE", "Syntax", "WBStartup", "NoCli",
                                         "CloseEd", "Data", "Dc", "Dcb", "Even", "IncBin" };

    if (l[s] == '#' || l[s] == '.')
        return 1;
    for (size_t i = 0; i < sizeof words / sizeof words[0]; i++)
        if (tp_word(l, s, e, words[i], NULL))
            return 1;
    return 0;
}

static void statement(struct fstate *f, const char *l, size_t len, size_t s, size_t e)
{
    size_t p;
    const char *name;
    size_t nlen;
    int b;

    if (f->in_macro) {
        if (tp_word(l, s, e, "End", &p) && tp_word(l, tp_blanks(l, p, e), e, "Macro", NULL))
            f->in_macro = 0;
        return;
    }
    if (f->in_newtype) {
        if (tp_word(l, s, e, "End", &p) && tp_word(l, tp_blanks(l, p, e), e, "NEWTYPE", NULL))
            f->in_newtype = 0;
        return;
    }
    if (tp_word(l, s, e, "CNIF", NULL) || tp_word(l, s, e, "CSIF", NULL)) {
        push_cn(f);
        return;
    }
    if (tp_word(l, s, e, "CELSE", NULL)) {
        if (f->ncn)
            f->cn[f->ncn - 1] = ++f->next_id;
        return;
    }
    if (tp_word(l, s, e, "CEND", NULL)) {
        if (f->ncn)
            f->ncn--;
        return;
    }
    if (f->in_proc) {
        if (tp_word(l, s, e, "End", &p)) {
            p = tp_blanks(l, p, e);
            if (tp_word(l, p, e, "Statement", NULL) || tp_word(l, p, e, "Function", NULL))
                f->in_proc = 0;
        }
        return;
    }
    if (tp_word(l, s, e, "Macro", &p)) {
        size_t q;

        p = tp_blanks(l, p, e);
        for (q = p; q < e && is_ident((unsigned char)l[q]); q++)
            ;
        f->in_macro = 1;
        free(f->macro_name);
        f->macro_name = tp_strndup(l + p, q - p);
        return;
    }
    if ((tp_word(l, s, e, "Statement", &p) || tp_word(l, s, e, "Function", &p))) {
        if (p < e && l[p] == '.')
            for (p++; p < e && is_ident((unsigned char)l[p]); p++)
                ;
        if (!tp_word(l, tp_blanks(l, p, e), e, "Return", NULL)) {
            f->in_proc = 1;
            return;
        }
    }
    if (tp_word(l, s, e, "NEWTYPE", NULL)) {
        f->in_newtype = 1;
        return;
    }
    b = tp_block_macro(l, s, e, len, &name, &nlen);
    switch (b) {
    case TP_B_INIT:
    case TP_B_CASE:
    case TP_B_FINAL:
        open_block(f, b, name, nlen);
        return;
    case TP_B_END_INIT:
    case TP_B_END_CASE:
    case TP_B_END_FINAL:
        end_block(f, b);
        return;
    }
    if (f->block == TP_B_NONE) {
        if (!declaration(l, s, e))
            ferr(f, f->line, "executable code outside the blocks: %.*s", (int)(e - s > 60 ? 60 : e - s), l + s);
        return;
    }
    control(f, l, s, e);
}

/* a block macro anywhere in the line must stand alone on it */
static void block_macros_alone(struct fstate *f, const char *l, size_t len, size_t comment)
{
    int quote = 0;

    for (size_t p = 0; p < comment; p++) {
        const char *name;
        size_t nlen, end;
        int b;

        if (l[p] == '"')
            quote = !quote;
        if (quote || l[p] != '!')
            continue;
        b = tp_block_macro(l, p, comment, len, &name, &nlen);
        if (b == TP_B_NONE)
            continue;
        if (f->in_macro) {
            ferr(f, f->line, "a block macro inside the macro %s", f->macro_name ? f->macro_name : "?");
            return;
        }
        end = p + 1;
        while (end < len && is_ident((unsigned char)l[end]))
            end++;
        nlen = end - p;
        if (b == TP_B_CASE) {
            size_t q = end, close;
            struct tp_stmt a[2];

            while (q < len && l[q] == ' ')
                q++;
            if (q < len && l[q] == '{' && tp_args(l, len, q, a, 2, &close) >= 0)
                end = close + 1;
        }
        end = tp_blanks(l, end, len);
        while (end < len && l[end] == '\r')
            end++;
        if (tp_blanks(l, 0, p) != p || (end < len && l[end] != ';')) {
            ferr(f, f->line, "%.*s must stand on a line of its own; only a comment may follow it", (int)nlen,
                 l + p);
        }
        return;
    }
}

int tp_check_form(const char *text, size_t len, const struct tp_params *par, struct tp_source *src)
{
    struct fstate f;
    const char *p = text, *end = text + len;

    memset(&f, 0, sizeof f);
    f.par = par;
    f.src = src;
    if (memchr(text, 0, len))
        return (int)src->errors.n;      /* tp_process said so */
    while (p < end) {
        const char *nl = memchr(p, '\n', (size_t)(end - p));
        size_t n = nl ? (size_t)(nl - p) : (size_t)(end - p);
        struct tp_stmt st[64];
        size_t comment, nst;

        f.line++;
        nst = tp_statements(p, n, st, 64, &comment);
        block_macros_alone(&f, p, n, comment);
        for (size_t i = 0; i < nst; i++)
            statement(&f, p, n, st[i].start, st[i].end);
        p = nl ? nl + 1 : end;
    }
    if (f.block != TP_B_NONE) {
        char what[200];

        open_text(&f, what, sizeof what);
        ferr(&f, f.block_line, "%s has no %s", what, closing_name(f.block));
    }
    if (!f.ncases)
        ferr(&f, 1, "the source has no case");
    for (size_t i = 0; i < f.nnames; i++)
        if ((!strcmp(f.names[i], "init") && f.init_line) || (!strcmp(f.names[i], "end") && f.final_line))
            ferr(&f, f.name_lines[i], "the case name %s is the namespace of the batch %s", f.names[i],
                 f.names[i][0] == 'i' ? "init" : "end");
    free(f.macro_name);
    free(f.case_name);
    free(f.cn);
    free(f.cn_open);
    free(f.ctl);
    for (size_t i = 0; i < f.nnames; i++)
        free(f.names[i]);
    free(f.names);
    free(f.name_lines);
    return f.oom ? -1 : (int)src->errors.n;
}
