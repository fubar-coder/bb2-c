/*
 * ab3tok.c - the mode of blitz2-ab3's program and the data of the mode
 * (ext.h): AmiBlitz3's token list (spec ab3-compat/libraries), the
 * library of the float power, and the tokens of AmiBlitz3 that are not
 * compiled yet.
 */
#include "blitz2.h"
#include "ab3.h"
#include "tokimg.h"

/* AmiBlitz3's list (toklist.h): the same nodes as blitz2's (main.c
   tokimg) with AmiBlitz3's names (M picks b3), blitz2's last one followed
   by AmiBlitz3's new ones */
#define TK(id, next, num, name, help)       TKMEMBER(id, name, help)
#define TA(id, nx, n, name, ...)            TAMEMBER(id, name)
#define TKLAST(id, num, name, help)         TKMEMBER(id, name, help)
#define TK3(id, next, num, name, help)      TKMEMBER(id, name, help)
#define TK3LAST(id, num, name, help)        TKMEMBER(id, name, help)
#define M(b2, b3) b3

struct tokimage_ab3 {
#include "toklist.h"
};

#undef TK
#undef TA
#undef TKLAST
#undef TK3
#undef TK3LAST
#define TK(id, next, num, name, help)       { NX(next), num, name "\0" help },
#define TA(id, next, num, name, ...)        { NX(next), num, name "\0" "", { __VA_ARGS__ } },
#define TKLAST(id, num, name, help)         { NX(toktnum34), num, name "\0" help },
#define TK3(id, next, num, name, help)      { NX(next), num, name "\0" help },
#define TK3LAST(id, num, name, help)        { NULL, num, name "\0" help },
#define IMG tokimg_ab3

static struct tokimage_ab3 tokimg_ab3 = {
#include "toklist.h"
};

int progmode = XTRA_MODE_BB2;           /* ab3.h */

/* the data of the mode (ext.h): BB2's until the command line decides */
struct token *ext_toklist = (struct token *)&tokimg;
UWORD ext_mathq = 247-55;

void ab3_setmode(int mode)
{
    progmode = mode;
    if (mode == XTRA_MODE_AB3) {
        ext_toklist = (struct token *)&tokimg_ab3;
        /* AmiBlitz3's math library is number 44 (AmiBlitz3.ab3 14565,
           token $1601), its 192 an OsLibs one */
        ext_mathq = 44;
    }
}

/* AmiBlitz3's tokens since Blitz Basic 2 (toklist.h, tnum+33..tnum+40,
   $80D4-$80DB) are known to the tokeniser of AB3 mode but not compiled
   yet (docs/ab3/variant.md, G2+): in AB3 mode one of them stops the
   compile with "Not supported: NAME" */
void ext_newtoken(UWORD tok)
{
    static char msg[sizeof "Not supported: " + 16] = "Not supported: ";
    const struct token *t;
    char *d = msg + sizeof "Not supported: " - 1;
    const char *s;

    if (progmode != XTRA_MODE_AB3 || tok < tnum + 33 || tok > tnum + 40)
        return;
    for (t = &tokens; t && t->num != tok; t = t->next)
        ;
    for (s = t ? t->name : "?"; *s && d < msg + sizeof msg - 1; )
        *d++ = *s++;
    *d = 0;
    err(msg);
}
