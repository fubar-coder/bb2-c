#include "corpusnotes.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *const passes[] = { "corpus", "corpus_runerr", "corpus_req", "corpus_serial",
                                      "corpus_serial_runerr" };

int cnotes_known_pass(const char *pass)
{
    size_t i;

    for (i = 0; i < sizeof passes / sizeof passes[0]; i++)
        if (!strcmp(pass, passes[i]))
            return 1;
    return 0;
}

/* a name: not empty, printable, none of the separators */
static int name_ok(const char *s)
{
    size_t i, n = strlen(s);

    for (i = 0; i < n; i++)
        if ((unsigned char)s[i] <= ' ' || (unsigned char)s[i] > 126 || strchr(":;|/\\=#", s[i]))
            return 0;
    return n > 0;
}

/* "#NNN: REASON": the issue (0 = not of the form) */
static unsigned note_issue(const char *t)
{
    char *end;
    unsigned long v;

    if (*t != '#' || t[1] < '1' || t[1] > '9')
        return 0;
    v = strtoul(t + 1, &end, 10);
    if (v > 999999 || end[0] != ':' || end[1] != ' ')
        return 0;
    for (end += 2; *end == ' '; end++)
        ;
    return *end ? (unsigned)v : 0;
}

static int bad(char *err, size_t errlen, unsigned line, const char *why)
{
    snprintf(err, errlen, "line %u: %s", line, why);
    return -1;
}

int cnotes_parse(struct cnotes *cn, const char *text, char *err, size_t errlen)
{
    const char *s = text;
    unsigned line = 0;

    while (*s) {
        const char *eol = strchr(s, '\n'), *t1, *t2;
        size_t len = eol ? (size_t)(eol - s) : strlen(s);
        char *row = strndup(s, len), *pass, *name, *note;
        struct cnote *v = NULL;
        size_t i;
        unsigned issue = 0;

        s += len + (eol != NULL);
        line++;
        if (!row)
            return bad(err, errlen, line, "out of memory");
        if (len && row[len - 1] == '\r')
            row[--len] = 0;
        if (!*row || *row == ' ' || *row == '#') {
            free(row);
            continue;
        }
        t1 = strchr(row, '\t');
        t2 = t1 ? strchr(t1 + 1, '\t') : NULL;
        if (!t2 || strchr(t2 + 1, '\t')) {
            free(row);
            return bad(err, errlen, line, "PASS<TAB>NAME<TAB>#NNN: REASON expected");
        }
        pass = strndup(row, (size_t)(t1 - row));
        name = strndup(t1 + 1, (size_t)(t2 - t1 - 1));
        note = strdup(t2 + 1);
        free(row);
        if (!pass || !name || !note) {
            free(pass);
            free(name);
            free(note);
            return bad(err, errlen, line, "out of memory");
        }
        if (!cnotes_known_pass(pass))
            snprintf(err, errlen, "line %u: unknown pass \"%s\" (corpus, corpus_runerr, "
                     "corpus_req, corpus_serial or corpus_serial_runerr)", line, pass);
        else if (!name_ok(name))
            snprintf(err, errlen, "line %u: the program name \"%s\" is empty or has a character "
                     "of : ; | / \\ = # or a blank", line, name);
        else if (!(issue = note_issue(note)))
            snprintf(err, errlen, "line %u: the note must read \"#NNN: REASON\" (the issue and "
                     "a reason)", line);
        else {
            for (i = 0; i < cn->n; i++)
                if (!strcmp(cn->e[i].pass, pass) && !strcmp(cn->e[i].name, name)) {
                    snprintf(err, errlen, "line %u: %s:%s is listed twice (line %u)", line, pass,
                             name, cn->e[i].line);
                    break;
                }
            if (i == cn->n && !(v = realloc(cn->e, (cn->n + 1) * sizeof *v)))
                snprintf(err, errlen, "line %u: out of memory", line);
            else if (i == cn->n) {
                struct cnote x = { pass, name, note, issue, line, 0, 0 };

                cn->e = v;
                cn->e[cn->n++] = x;
                continue;
            }
        }
        free(pass);
        free(name);
        free(note);
        return -1;
    }
    return 0;
}

void cnotes_free(struct cnotes *cn)
{
    size_t i;

    for (i = 0; i < cn->n; i++) {
        free(cn->e[i].pass);
        free(cn->e[i].name);
        free(cn->e[i].text);
    }
    free(cn->e);
    memset(cn, 0, sizeof *cn);
}

int cnotes_judge(struct cnotes *cn, const char *pass, const char *name, int differs,
                 const struct cnote **note)
{
    size_t i;

    if (note)
        *note = NULL;
    for (i = 0; i < cn->n; i++) {
        struct cnote *e = &cn->e[i];

        if (strcmp(e->pass, pass) || strcmp(e->name, name))
            continue;
        e->seen = 1;
        if (!differs)
            return CNOTE_SAME;
        e->differed = 1;
        if (note)
            *note = e;
        return CNOTE_NOTED;
    }
    return differs ? CNOTE_DIFFERS : CNOTE_SAME;
}

const struct cnote *cnotes_stale(const struct cnotes *cn, const char *pass, size_t *cursor)
{
    while (*cursor < cn->n) {
        const struct cnote *e = &cn->e[(*cursor)++];

        if (!strcmp(e->pass, pass) && !e->differed)
            return e;
    }
    return NULL;
}
