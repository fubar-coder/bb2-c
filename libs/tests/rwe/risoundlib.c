/*
 * rwe risoundlib - CludgeSound: the C build or the CD original (BSS2
 * RISOUNDLIB.OBJ), one set as LIBSET says, observed as cases (rwecmp.h;
 * the expected data libs/tests/expected/rwe_risoundlib.txt).
 *
 *   risoundlib DIR     (DIR with memlib, risoundlib .obj/.orig)
 *
 * The cases are those of risoundcludge.h (the same routine is in
 * risoundlib2, whose test runs them too); memlib's memory all back at the
 * end.
 */
#include "rwecmp.h"
#include "risoundcludge.h"

#define LIB "risoundlib"

static void memlib(int idx)
{
    markregs(&R);
    callstk(entry_form(getlib(&loaded, "memlib"), idx, 0), &R, NULL, 0, &POP);
}

int main(int argc, char **argv)
{
    static const char *names[] = { "memlib", LIB, NULL };
    ULONG avail;

    if (argc < 2) {
        printf("usage: risoundlib DIR\n");
        return 20;
    }
    if (!load_set(argv[1], names))
        return 20;
    KEEPS("CludgeSound", d3-d7|a2|a3|a6);

    avail = AvailMem(MEMF_ANY);
    rwe_mem_start();
    ROUT = "memlib init";
    memlib(7);

    cs_cases(LIB, 0);

    ROUT = "memlib finit";
    memlib(8);
    rwe_mem_check(LIB, avail, TRUE);
    return rwe_done(LIB);
}
