/*
 * cmdref.c - the command table of a library file as JSON, see cmdref.h.
 */
#include <string.h>
#include "cmdref.h"

static int printable(uint8_t c)
{
    return c >= 0x20 && c < 0x7F;
}

static int ends_with(const char *s, size_t n, const char *suffix)
{
    size_t k = strlen(suffix);

    return n > k && !memcmp(s + n - k, suffix, k);
}

int cmdref_opens(const struct blitzlib *lib, const char **name, char *err, size_t errlen)
{
    static const char *const suffixes[] = { ".library", ".resource", ".device", ".gadget" };
    static const uint8_t zero[4];
    uint32_t p = lib->codestart;

    *name = NULL;
    if (lib->size < 4 || memcmp(lib->code, zero, 4))
        return 0;                       /* not an AmigaOS call library */
    while (p < lib->size) {
        uint32_t start = p;
        size_t i;

        while (p < lib->size && printable(lib->code[p]))
            p++;
        if (p < lib->size && p > start && !lib->code[p]) {
            const char *s = (const char *)lib->code + start;

            for (i = 0; i < sizeof suffixes / sizeof *suffixes; i++)
                if (ends_with(s, p - start, suffixes[i]))
                    break;
            if (i < sizeof suffixes / sizeof *suffixes) {
                if (*name) {
                    snprintf(err, errlen, "two library names in the code: \"%s\" and \"%s\"",
                             *name, s);
                    *name = NULL;
                    return -1;
                }
                *name = s;
            }
        }
        p++;
    }
    return 0;
}

/* a JSON string: printable ASCII as it is (" and \ escaped), every other
   byte as \u00XX */
static void str(FILE *f, const char *s)
{
    const uint8_t *p = (const uint8_t *)s;

    putc('"', f);
    for (; *p; p++) {
        if (*p == '"' || *p == '\\')
            fprintf(f, "\\%c", *p);
        else if (printable(*p))
            putc(*p, f);
        else
            fprintf(f, "\\u%04x", *p);
    }
    putc('"', f);
}

/* the word of a type (the low 4 bits of an argument or the result) */
static const char *type_word(unsigned t)
{
    static const char *const words[] = {
        "suffix", "byte", "word", "long", "quick", "float", "type6", "string", "any"
    };

    return t < sizeof words / sizeof *words ? words[t] : "unknown";
}

static void arg(FILE *f, uint8_t t)
{
    const char *flags[4];
    unsigned n = 0, i;

    fputs("{\"type\": ", f);
    if (t & 0x20) {                     /* array (handlearr) */
        str(f, "array");
        if (t & 0x08)
            flags[n++] = "arraytype";
        if (t & 0x10)
            flags[n++] = "arrayend";
    } else if (t & 0x80) {              /* address of a variable (handlevara) */
        str(f, (t & 7) ? type_word(t & 7) : "any");
        flags[n++] = "address";
    } else {
        str(f, type_word(t & 0x0F));
    }
    if (t & 0x40)
        flags[n++] = "stack";
    fprintf(f, ", \"code\": %u", t);
    if (n) {
        fputs(", \"flags\": [", f);
        for (i = 0; i < n; i++) {
            if (i)
                fputs(", ", f);
            str(f, flags[i]);
        }
        putc(']', f);
    }
    putc('}', f);
}

static void form(FILE *f, const struct blitzlib_form *fm, int last)
{
    unsigned count = fm->head & 0xFF, from = fm->head >> 12, num = (fm->head >> 8) & 0xF, i;

    fputs("        {\"args\": [", f);
    for (i = 0; i < count; i++) {
        if (i)
            fputs(", ", f);
        arg(f, fm->types[i]);
    }
    putc(']', f);
    if (num)
        fprintf(f, ", \"repeat\": {\"from\": %u, \"count\": %u}", from, num);
    fputs(last ? "}\n" : "},\n", f);
}

static const char *kind_word(const struct blitzlib *lib, const struct blitzlib_entry *e)
{
    static const char *const words[] = { "kind0", "statement", "function", "command" };

    if (e->kind == 8)
        return e->name == lib->object ? "object" : "keyword";
    if (e->kind & 4)
        return "oscall";
    return words[e->kind & 3];
}

static void entry(FILE *f, const struct blitzlib *lib, const struct blitzlib_entry *e, int last)
{
    const char *kind = kind_word(lib, e);
    unsigned i;

    fputs("    {\n      \"name\": ", f);
    str(f, e->name);
    fputs(",\n      \"kind\": ", f);
    str(f, kind);
    if (!strcmp(kind, "oscall")) {
        fputs(",\n      \"help\": ", f);
        str(f, e->help);
        fprintf(f, ",\n      \"base\": %u,\n      \"offset\": %d,\n      \"registers\": [",
                e->oslib, e->lvo);
        for (i = 0; i < e->nregs; i++)
            fprintf(f, "%s\"%c%u\"", i ? ", " : "", e->regs[i] & 0x10 ? 'a' : 'd',
                    e->regs[i] & 7);
        fputs("]\n", f);
    } else if (e->kind == 8) {
        fputs(",\n      \"help\": ", f);
        str(f, e->help);
        putc('\n', f);
    } else {
        unsigned rtype = e->kind >> 8;

        if (e->kind & 8)
            fputs(",\n      \"typed\": true", f);
        fputs(",\n      \"help\": ", f);
        str(f, e->help);
        if ((e->kind & 3) >= 2) {
            fputs(",\n      \"result\": ", f);
            str(f, type_word(rtype));
        }
        fputs(",\n      \"forms\": [\n", f);
        for (i = 0; i < e->nforms; i++)
            form(f, &lib->forms[e->form + i], i + 1 == e->nforms);
        fputs("      ]\n", f);
    }
    fputs(last ? "    }\n" : "    },\n", f);
}

int cmdref_write(FILE *out, const struct blitzlib *lib, const char *group, const char *name,
                 const char *opens)
{
    unsigned i, n = 0, k = 0;

    fputs("{\n  \"name\": ", out);
    str(out, name);
    fputs(",\n  \"group\": ", out);
    str(out, group);
    fprintf(out, ",\n  \"number\": %u", lib->libnum);
    if (opens) {
        fputs(",\n  \"opens\": ", out);
        str(out, opens);
    }
    if (lib->object) {
        fputs(",\n  \"object\": {\"name\": ", out);
        str(out, lib->object);
        fprintf(out, ", \"maximum\": %u}", lib->defmax);
    }
    for (i = 0; i < lib->nentries; i++)
        n += lib->entries[i].name != NULL;
    fputs(",\n  \"entries\": [", out);
    fputs(n ? "\n" : "", out);
    for (i = 0; i < lib->nentries; i++)
        if (lib->entries[i].name)
            entry(out, lib, &lib->entries[i], ++k == n);
    fputs(n ? "  ]\n}\n" : "]\n}\n", out);
    return ferror(out) ? -1 : 0;
}
