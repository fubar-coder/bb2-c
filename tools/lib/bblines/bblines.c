/*
 * bblines.c - edits of the lines of a tokenised source, see bblines.h.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bblines.h"

int bblines_comment(const uint8_t *src, size_t len, uint16_t token, uint8_t **out,
                    size_t *outlen, size_t *count, char *err, size_t errlen)
{
    uint8_t *o;
    size_t i = 0, n = 0, line = 0;

    *out = NULL;
    *outlen = 0;
    *count = 0;
    if (!(token & 0x8000)) {
        snprintf(err, errlen, "$%04X is not a token word (bit 15)", token);
        return -1;
    }
    if (len && !memchr(src, 0, len)) {
        snprintf(err, errlen, "not tokenised (no 0 line ends)");
        return -1;
    }
    if (!(o = malloc(len + 1))) {
        snprintf(err, errlen, "out of memory");
        return -1;
    }
    while (i < len) {
        size_t start = i, k = i, end;
        int quote = 0, match = 0, colon = 0;

        line++;
        /* the line's end: 0 outside a token word */
        for (end = i; end < len && src[end]; end++)
            if (src[end] & 0x80) {
                if (end + 1 >= len) {
                    snprintf(err, errlen, "line %zu: a token without its second byte", line);
                    free(o);
                    return -1;
                }
                end++;
            }
        if (end >= len) {
            snprintf(err, errlen, "line %zu: no line end", line);
            free(o);
            return -1;
        }
        while (k < end && (src[k] == ' ' || src[k] == '\t'))
            k++;
        if (k < end && (src[k] & 0x80) && (uint16_t)(src[k] << 8 | src[k + 1]) == token)
            match = 1;
        if (match) {
            for (k += 2; k < end; k++) {
                if (src[k] & 0x80) {
                    k++;
                    continue;
                }
                if (src[k] == '"')
                    quote = !quote;
                else if (src[k] == ':' && !quote)
                    colon = 1;
                else if (src[k] == ';' && !quote)
                    break;              /* a comment: the rest is text */
            }
            if (colon) {
                snprintf(err, errlen, "line %zu: the token $%04X with more statements", line,
                         token);
                free(o);
                return -1;
            }
            o[n++] = ';';
            o[n++] = 0;
            (*count)++;
        } else {
            memcpy(o + n, src + start, end + 1 - start);
            n += end + 1 - start;
        }
        i = end + 1;
    }
    *out = o;
    *outlen = n;
    return 0;
}
