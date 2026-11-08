# Variants and userlibs of the libraries

**Variants** of the libraries: a directory `libs/variants/NAME/` with a
`libnums.h` (the library numbers) is a variant. The default variant
(`-DBLITZ_DEFAULT_VARIANT=NAME`, default `bb2-2.10`: the numbers of the
CD's `libnums.bb`) is built to `build/amiga/blitz2/acidlibs` and
`deflibs` and checked against the CD; every other one to
`build/amiga/variants/NAME/acidlibs` and `deflibs` (targets
`NAME-acidlibs`, `NAME-deflibs`). A variant's `libnums.h` can include the
one of another (`../bb2-2.10/libnums.h`) and change single numbers
(`#undef`/`#define LIB_xxx`). Every number in the libraries comes from
`libnums.h` – header, `!libs` references, the calls of other libraries
(ALibJsr) – so a changed number moves all of them, and the tokens of a
source written for the variant come from its files (`blitztokens`,
`tedsrc detok --blitz2 … --lib …`). Test variants are in
`libs/tests/variants/` (ctest `variant_*`).

**Userlibs** of a variant: `libs/userlibs/DIR/NAME.c` →
`build/amiga/variants/VARIANT/userlibs/DIR/NAME.obj` (for
`blitz2:userlibs/DIR/`). `libs/userlibs/rwe/`: the libraries of Red When
Excited's sources (`docs/rwelibs/README.md`), variant `rwe-1999`; the
list `RWE_LIBS` there names per library the Blitz source and its CD
binary. They are checked like the acidlibs: declarations equal to the
binary's (ctest `rwe_declarations_NAME`, `rwe_numbers`), behaviour
compared with it in Amiberry (`libs/tests/rwe/NAME.c`, test cases
`rwe NAME` of `libs_emulator`). A library whose number the acidlibs
have is built in a variant of its own (`RWE_OWN_VARIANTS`):
`rwe-1999-eval` is `rwe-1999` with polylib moved from 121 to 87
(`#renderlib`) for RIEvalLib (121,
`variants/rwe-1999-eval/userlibs/rwe/rievallib.obj`, used with the RWE
userlibs of `rwe-1999`, whose numbers are the same; ctest
`variant_rwe_1999_eval`, `rwe_numbers_rwe-1999-eval`); `rwe-1999-ahi`
has `rwe-1999`'s numbers and risoundlib2 instead of risoundlib (both 45:
`variants/rwe-1999-ahi/userlibs/rwe/risoundlib2.obj`, used with the RWE
userlibs of `rwe-1999` but `risoundlib.obj`; ctest
`variant_rwe_1999_ahi`, `rwe_numbers_rwe-1999-ahi`). Target
`rwelibsrc`: those Blitz sources as TED shows them, in
`build/amiga/rwelibsrc/` (like `bblibsrc`), with the tokens of the
library files of their variant built here (blitz2, the variant's
acidlibs and deflibs, all RWE userlibs; needs the Aminet archive, not
the CD). The **reference build** of
those sources (`libs/userlibs/rwe/refbuild.cmake`,
`docs/rwelibs/README.md` §3.1): the build stages them (target
`rwe_refstage`: `build/amiga/libs/userlibs/rwe/refstage/` with the CD's
`EXTRAS/BLITZLIBS/`, the residents `lvo.res`, `custom.res`,
`datatypes.res`, `AHI.res` that `tools/blitzres` writes from the toolchain's NDK
(target `rwe_residents`), a `LIBNUMS.BB` with the numbers of the
variant's `libnums.h`, the sources with their `INCDIR` lines neutralised
by `tools/bblines`), ctest `rwe_refbuild` compiles them with the CD's
blitz2 2.10 in Amiberry to `build/amiga/libs/userlibs/rwe/ref/NAME.obj`.
An `RWE_LIBS` entry with REFERENCE `refbuild` is compared with that
file.
