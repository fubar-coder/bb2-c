# Deviations of the C compiler from Blitz Basic 2 2.10

Reference: the compiler of the CD (2.10), built from `blitz2.s`
(`archive/`, [../v2.10/README.md](../v2.10/README.md)). This file lists every
place where `programs/blitz2` (and the code in `shared/` it uses)
behaves differently from it. Rule (CLAUDE.md "Original bugs are fixed
deliberately"): a bug of the original not fixed yet is marked
`BUG(orig) (#N)` in the code; a fix is an intended deviation of kind **F**,
listed here with its issue and its test; every other deviation is marked
`DIFF:` in the code with its reason.
Categories as in [`../libs/deviations.md`](../libs/deviations.md) (**B** decided
by the user, **T** technically unavoidable, **K** silent correction - no
longer permitted, **F** fixed original bug).

[`../libs/deviations.md`](../libs/deviations.md) section 5 holds the long
description of most of these places (original, C version, what a program
can observe); the entries here give the place, the effect and the tests of
every issue in one list. The reasons of the `DIFF:` notes are at their
comments in `programs/blitz2/src/`. Fixes that belong to the libraries or
to TED (for example #6, #999, #1063; #154 to #160, #345 and #1042 of the
file requester; #357 and #387 of TED's start) are listed in
[`../libs/deviations.md`](../libs/deviations.md) and
[`../ted/deviations.md`](../ted/deviations.md), not here.

## 1. Fixed original bugs (kind F)

One entry per issue, in the order of the issue numbers. The tests are
ctest tests (`tests/README.md`).

| Issue | Place | What changes | Tests |
|---|---|---|---|
| #1 | insasm, freeasms (asmlibs.c:456, :1583, blitz2.s:5435, :6701); struct asmnode (structs.h) | An inline assembler expression of 240 characters or more in a macro or include is freed correctly; the original freed it with a wrong size. | `compiler-insasm-size-byte` |
| #3 | loadalib (asmlibs.c:1358, blitz2.s:6470) | A library file that cannot be read completely is left out; the original ignored the read error and used whatever was in its buffer. | `compiler-loadalib-readerr` |
| #7 | handleconst (compile.c:1332, blitz2.s:3065) | A constant definition without a name, `#=5`, is "Syntax Error"; the original accepted it. | `compiler-const-emptyname` |
| #8 | optreqprep, optreqdone (data.c:1756, :1769, options.src:1-18) | Without chip memory for the options requester's arrows, blitz2 gives up at the start with return code -1; the original wrote to and freed address 0. | `compiler-optreqprep-allocmem` |
| #9 | eval2 (eval.c:874, blitz2.s:9125) | `=<` is less-or-equal, the same as `<=`; the original compiled it as "not equal" without a message. | `compiler-eval-eqlt-operator` |
| #10 | fetchpoint2 (libcall.c:123, blitz2.s:10737) | A number ending in a decimal point, such as `1.`, compiles as a number; the original reported a syntax error. | `compiler-float-trailing-dot` |
| #11 | fetchpoint2 (libcall.c:123, blitz2.s:10745) | An exponent after a fraction of 0, as in `1.0e5`, is applied; the original ignored it and gave 1. | `compiler-float-zerofrac-exponent` |
| #13 | consttype2 `.quick2` (libcall.c:440, blitz2.s:10961) | A number without a fraction in a Quick calculation, such as `q=1.0`, is a Quick constant; the original converted a float at run time (same value, longer code). | `compiler-consttype-zero-negative-frac` |
| #14 | pushdo, pushput (libcall.c:430, :455, blitz2.s:11028, :11057) | A full object buffer during an assignment or `Select` loses no memory; the original lost a little. | `compiler-pushput-leak` |
| #15 | muld1 (libcall.c:519, blitz2.s:11106) | Arrays with 256-byte elements have their elements 256 bytes apart; the original put them 65536 bytes apart and overwrote memory. | `compiler-muld1-x256` |
| #16 | doincdir (libcall.c:1264, blitz2.s:11936) | An `IncDir` path of 96 characters or more is "Path too Long"; the original overwrote the compiler's data and the program crashed. | `compiler-incdir-overflow` |
| #17 | doincdir, makeiname (libcall.c:1264, :1291, blitz2.s:11936, :11947); doincbin, doinclude, doxinclude (tokens1.c:824, :866, :802) | A quoted path of `IncDir`, `Include`, `XInclude` or `IncBin` ends at its closing quote, other text after it is "Syntax Error"; the original took the whole line. | `compiler-incdir-rest-of-line`, `compiler-incdir-quote-next`, `compiler-include-quote-colon`, `compiler-incdir-quote-garbage`, `compiler-incdir-unquoted`, `compiler-incdir-quote-else` |
| #19 | Workbench start (main.c:566, wbheader:37) | Started from the Workbench, blitz2 closes a short or damaged icon file after reading; the original kept it in use until the reboot. | `compiler-wb-info-shortread` |
| #20 | Workbench start (main.c:578, wbheader:28-71) | Started from the Workbench without a project icon, the editor starts with no program; the original could ask it to load its own icon as a program. | `compiler-wb-info-progfile` |
| #22 | blitz2's exit (main.c:618, blitz2.s:1479, wbheader:88) | blitz2 returns -1 to the shell when it cannot start and 0 after a normal session; the original always returned 0. | `compiler-wbexit-returncode` |
| #23 | loaddmod (main.c:802, blitz2.s:1528) | LOAD DEBUG MODULE resets the busy pointer also when no debugger module was loaded before; the original left the busy pointer on. | `compiler-loaddmod-pointer` |
| #25 | menu10 (main.c:917, blitz2.s:1605) | When RUN or COMPILE AND RUN cannot start the program for lack of memory, the `EXECUTING` requester closes; the original left it open. | `compiler-createproc-fail` |
| #26 | flushlibs (main.c:1037, blitz2.s:1765) | With 256 MB or more free memory, blitz2 frees its memory test request at once; the original lost 256 MB when it ended. | `compiler-flushlibs-leak` |
| #27 | checklib (main.c:1061, blitz2.s:1784) | When blitz2 cannot examine `blitz2:deflibs` or `blitz2:acidlibs`, the file counts as missing; the original loaded it with the previous file's length. | `compiler-checklib-examine` |
| #28 | makeblitzlibs (main.c:1092, blitz2.s:1816; structs.h libsnode) | A directory in `blitz2:userlibs` with a name of 32 characters or more is skipped; the original overwrote memory. | `compiler-libsnode-name` |
| #29 | makename4 (parse.c:833, blitz2.s:7723) | A name of more than 127 characters, possible through a macro, is "Syntax Error"; the original overwrote the compiler's data. | `compiler-makename-overflow` |
| #30 | findlab (parse.c:909, blitz2.s:7822); fetchconst, fetchasm, makealab (eval.c, blitz2.s:9896, :10460, :10352); handlemouse (compile.c, blitz2.s:2996) | A `#`, `Goto`, `Gosub`, `Restore`, `?` or `.` without a name is "Syntax Error"; the original searched 65536 bytes of memory for an empty name. | `compiler-findlab-emptyname`, `compiler-findlab-emptyname-goto` |
| #31 | dousepath (stmts.c:129, blitz2.s:3306) | A `UsePath` path of 128 characters or more is "Path too Long"; the original overwrote the compiler's own code. | `compiler-usepath-overflow` |
| #32 | newallox (stmts.c, blitz2.s:3969) | When the larger buffers of INCREASE 150% - RECOMPILE cannot be allocated, the compile ends with the out of memory notice; the original's code compiled again with the old sizes and showed the same requester again and again, a case the original itself never reached. | `compiler-newallox-fallback` |
| #34 | fixconsts (stmts.c:1105) | The constant list of a resident ends inside the resident; the original's last constant pointed behind it, and every compile freed foreign memory. | `compiler-resconsts-last-next` |
| #35 | fixtypes (stmts.c:1157, blitz2.s:4521) | A resident with a `NewType` without fields loads everything defined behind it; the original lost those types, macros and constants. | `compiler-fixtypes-nofields` |
| #36 | loadres, checkconstclash (stmts.c:1387, :1319, blitz2.s:4393, :4441) | Two residents defining the same constant differently give the warning "Clash in Residents" and use the first value; the original used it silently. | `compiler-loadres-clash-offset`, `ab3_cli` |
| #37 | dodcb (tokens1.c:522, blitz2.s:12501) | `Dcb` with a count of 0 emits nothing and a negative count is "Illegal Constant"; the original made about four thousand million copies. | `compiler-dcb-zero`, `compiler-dcb-negative` |
| #38 | doincbin (tokens1.c:824) | `IncBin` releases its file or directory after an error; the original kept it in use. | `compiler-incbin-leaks`, `compiler-incbin-leaks-readerr` |
| #39 | dolet2 (tokens2.c:1238, blitz2.s:14386) | `For` with a global string variable is "Bad Type for For...Next"; the original compiled it and treated the string as a number. | `compiler-for-global-string` |
| #42 | calcvbr (main.c: `calcvbr`) | On a 68010 with a moved vector table, runtime errors and the debugger use that table; the original took the trap vectors from the wrong one. |  |
| #46 | libfunction, liba3 (eval.c:2125, :2085, blitz2.s:10112) | `PeekSound` inside a string expression leaves the sound object intact; the original overwrote it. | `compiler-libfunc-a3-string` |
| #325 | makeiname (libcall.c:1291, blitz2.s:11947) | An include file name that is more than 127 characters with the `IncDir` path is "Path too Long"; the original overwrote the compiler's data. | `compiler-makeiname-namebuff` |
| #335 | end of the compile, calcmaxs (compile.c:869, blitz2.s:2654, :2665, :2750) | A program using a library that needs objects only through another library ends cleanly; the original could hang the system at its end. | `compiler-maxs-libjsr` |
| #336 | blitz2's exit, fail2b (main.c:527); makemacbuff (stmts.c:784), called by loadxtra and optsdone | The Macro Buffer option sets the largest macro text, a longer one is "Macro too Big"; the original ignored the option and could crash when ending. | `compiler-macrobuff-freemem`, `compiler-macrobuff-freemem-large`, `ab3_cli`, `compiler-macrobuff-size`, `compiler-macrobuff-size-small` |
| #341 | makeiname (libcall.c:1291, blitz2.s:11947) | After `IncDir`, a file name with a device or assign is included as given; the original put the `IncDir` path in front and failed. | `compiler-makeiname-absolute` |
| #342 | setcvars (compile.c:711), incdir (data.c:60, blitz2.s:14862) | The path of `IncDir` applies only to the compile that set it; the original kept it for the following compiles. | `compiler-incdir-leak` |
| #343 | eval5 (eval.c:790, blitz2.s:9025); getelement2 (eval.c, blitz2.s:9737) | Character constants in inline assembler are read correctly on every line; after a calculation such as `#">"*8`, the original could make an earlier `#" "` 0. | `compiler-asmfixer-quoteflag` |
| #344 | amigalib, amigasave (eval.c:2004, :1983, blitz2.s:10183) | An AmigaOS function called inside an expression keeps the expression's intermediate values; the original overwrote them. | `compiler-amigalib-param-regs` |
| #369 | doallocmem and its sites (stmts.c `doallocmem`, `doallocerr`, blitz2.s:3379; parse.c `nomemerr`, `nomemshow`; makeallox, makeblitzlibs, loadlibgroup, loadalib, droplocals, domacro, compile) | Out of memory during a compile from the editor ends the compile through the compiler's own cleanup, with include files closed and memory freed, and the editor shows its alert; the original abandoned the compile in the middle, and the next compile did not come back. | `ted-outmem-in-blitz2-call` |
| #937 | calcvar (parse.c:1262, blitz2.s:8214-8240) | A List array indexed by an element of another array reads the right element; the original used a wrong element size. | `compiler-calcvar-listoff` |
| #939 | getmaxel, dofree, douse (tokens1.c:205, :264, :276, blitz2.s:12196, :12253, :12266); doaddr (eval.c:1930, blitz2.s:10053); toreg (libcall.c:1077, blitz2.s:11647) | Objects 32 KB or more into their table are addressed correctly with a raised object maximum; the original addressed memory before the table. | `compiler-object-index-word`, `compiler-object-index-long`, `compiler-object-index-below` |
| #940 | doend3, stateret, funcret (tokens2.c:976, :223, :233, blitz2.s:13899, :13466, :13469) | Ending or leaving a procedure with the statement of the other kind is "Illegal End Procedure"; the original compiled it and the program crashed. | `compiler-procend-kind`, `compiler-procend-kind-endfunc`, `compiler-procend-kind-stateret`, `compiler-procend-kind-funcret`, `compiler-procend-kind-match` |
| #942 | domacro .gather (tokens1.c:959, blitz2.s:12863) | A one-line macro with a colon right after its name, `Macro a:1:End Macro`, expands to `1`; the original kept the colon in the text. | `compiler-macro-leading-colon` |
| #943 | savefile HUNK_RELOC32 (parse.c:243) | Very large executables with very many relocations load on AmigaOS 2.0 and later; the original wrote one relocation block that did not load. | `compiler-reloc32-one-block` |
| #958 | stamigalib, amigalib (stmts.c:629, eval.c:2004) | AmigaOS function calls with an argument in register a4 or a5 keep the program's variables; the original crashed or destroyed them. | `compiler-amigalib-a4a5` |
| #960 | fixlibs, findtoke (asmlibs.c:1491, :1203, blitz2.s:6525, :6277) | `Supervisor_` compiles; the original gave "Syntax Error" because it looked the name up in the dos library. | `compiler-supervisor-token` |
| #964 | domacro .cdone, macputb (tokens1.c:1045, :913, blitz2.s:12916, :12936); freemacs (asmlibs.c:1567, blitz2.s:6680) | A macro text of 65536 bytes or more is "Macro too Big"; the original cut it and lost part of its memory until a reboot. | `compiler-macro-textlen-word`, `compiler-macro-textlen-max` |
| #965 | domacro (tokens1.c:945) | An error inside a macro definition is shown; the original could crash the compiler freeing a macro text it never set. | `compiler-macro-uninit-free`, `compiler-macro-uninit-free-noend` |
| #966 | getchar `.cmake` (parse.c:432) | A `~` inside a string literal is an ordinary character; the original evaluated `~...~` there as a constant expression, so `"a~1+2~b"` became `a3b`. | `compiler-cmake-tilde-in-string`, `ab3_cli`, `ab3_cli_ref` |
| #968 | domacro `.notem` (tokens1.c:988, blitz2.s:12884-12893) | A macro line ending in `End` keeps the rest of the macro and of the calling line, as in AmiBlitz3; the original cut both off. | `compiler-macro-end-line`, `compiler-macro-end-line-include` |
| #969 | zeropc: doeven4, doeven8, alignpc/doeven, dods (tokens1.c:582), pokewd (eval.c:210); litstring (eval.c:2237), dodata (tokens1.c:751) | Pad bytes from `Even`, alignment and `Ds` are zero; the original filled them with random bytes. | `compiler-even-pad-byte`, `compiler-even-pad-byte-code`, `compiler-data-pad-byte` |
| #970 | Create Resident: writeitem, writeoffs, restypes, resmacs, resconsts (stmts.c:890); addhere (parse.c:882), domacro (tokens1.c:1048) | CREATE RESIDENT writes the same file each time, with unused bytes zero; the original wrote bytes it never set. | `compiler-resident-stale-bytes` |
| #971 | addstrings (eval.c:1139), getelement (eval.c:1448) | A string addition nested seven levels deep, or in a late register argument such as the text of `AddBox`, gets the right length; the original's was wrong. | `compiler-stradd-arg7` |
| #972 | handleconst (compile.c:1332, blitz2.s:3065); addhere (parse.c:882) | A constant defined without a value, a line `#c` alone, is 0; the original gave it a random value. | `compiler-const-no-value`, `ab3_cli_const_novalue` |
| #973 | typemode (tokens2.c:756) | A NewType field of the type being defined is "Illegal Type", a pointer to it stays allowed; the original gave the field a random size. | `compiler-newtype-self-field`, `compiler-newtype-self-field-untyped` |
| #974 | typemode, doend3 (tokens2.c:756, :976, blitz2.s:13740, :13882); calcvar, calcstatic2 (parse.c:1363, :1399, :160); fetchsize (eval.c:1606); dimwit (tokens2.c:1426); pointchk (compile.c:880) | A NewType field of a type defined later gets that type's size, and using it too early is "Type not yet defined"; the original gave a wrong size. | `compiler-newtype-forward-field`, `compiler-newtype-forward-undefined`, `compiler-newtype-forward-cycle`, `compiler-newtype-forward-cycle3`, `compiler-newtype-forward-mixed`, `compiler-newtype-forward-transitive`, `compiler-newtype-forward-resident`, `compiler-newtype-forward-early-field`, `compiler-newtype-forward-early-sizeof`, `compiler-newtype-forward-early-dim`, `compiler-newtype-forward-early-local` |
| #977 | makealab2 (eval.c:2256, blitz2.s:10363) | A jump to a label named like a constant of value 0 is "Label has been used as a Constant"; the original gave a nonsense error or touched random memory. | `compiler-const0-label` |
| #978 | eval5, varcont (eval.c:795, :2439, blitz2.s:8983, :10479); fetchnum3, consttype2 (libcall.c:248, :386, blitz2.s:10856, :10940) | A Byte variable compared with a constant from 128 to 255 matches when it holds that byte; the original comparison never matched. | `compiler-byte-const-compare` |
| #979 | end of the compile (compile.c:827, blitz2.s:2640) | A program ending with data of odd length ends normally; the original put its end code on an odd address and crashed. | `compiler-incbin-odd-end` |
| #980 | racing stripe: compile (compile.c:218), nextline (compile.c:348, :361), blitz2.s:1943, :2069, :2088; TED-C progress (screen.c, comdata+98/102) | TED draws the compile progress bar itself, also on wide RTG screens; the original compiler wrote about 28 KB before TED's screen memory there. | `compiler-racing-stripe-wide` |
| #993 | loadalib (asmlibs.c:1358, blitz2.s:6517) | A library file whose relocation data cannot be read is left out; the original used it and patched random places. | `compiler-loadalib-relocread` |
| #994 | pushdo, pushput (libcall.c:430, :455, blitz2.s:11028, :11057); qfree (compile.c:275, blitz2.s:1998) | A compile error in some assignments or in `Select` loses no memory; the original lost its code save buffer. | `compiler-pushdo-error-leak` |
| #995 | resmacs (stmts.c), fixmacs (stmts.c) | A resident with a macro text of 32767 bytes or more loads correctly; the original read the definitions behind it from the wrong place. | `compiler-resmacs-textlen-sign`, `compiler-resmacs-textlen-max` |
| #997 | asmconst, copyconst (asmlibs.c:45, :93, blitz2.s:5000, :5028) | An inline assembler operand too long for its buffer is "Operand too Long"; the original cut it into wrong code or overwrote the compiler's data. | `compiler-copyconst-overflow` |
| #998 | doused (eval.c:1885, blitz2.s:9989) | `Used` gives the right object number with a raised object maximum; the original gave a wrong number from 64 KB into the object table. | `compiler-object-used-word`, `compiler-object-used-below` |
| #1008 | ed_openreq users in `menu10` (compile.c), start in `main` (main.c) | With TED-C, a requester that cannot be opened, or memory that runs out while blitz2 starts, is reported by TED's alert and blitz2 goes on; the original jumped to `outmem` with a7 = 0 and crashed the system. TED's side of #1008 is in [../ted/deviations.md](../ted/deviations.md), row `outmem`/`memcont`. | `ted-outmem-blitz2-init`, `ted-outmem-blitz2-init-verreq` |
| #1044 | eval5, divzerochk (eval.c:835, :981, blitz2.s:9007-9038; evalconst2, libcall.c:1232, blitz2.s:11880) | A division or `Mod` by a constant 0 is the compile error "Division by Zero"; the original crashed the compiler or compiled the largest Long number. | `compiler-constdiv-word`, `compiler-constdiv-nonzero` |
| #1048 | tokecode, tokechx (asmlibs.c:1050, :1039, blitz2.s:6047) | Without runtime error checking, `TokeJsr` and `ALibJsr` call a routine with an error check directly; the original went through an extra 8-byte stub. | `compiler-tokecode-stub-release`, `compiler-tokecode-stub-debug` |
| #1049 | eval5, the cleanup of a fold (eval.c:878, blitz2.s:9060-9075) | A constant operation such as `7 Mod 3` as the first use of a library keeps that library's relocations; the original dropped them, and a later `Mod` could crash. | `compiler-fold-mod-relocs` |
| #1068 | asmit size suffix (asmlibs.c, ab3syn.c `ext_asmshortbra`) | AB3 mode: the size suffix `.s` of an assembler instruction is read as `.b`, and BRA/BSR take every size suffix, as in AmiBlitz3 v3.14.0; the port refused `.s` (a bug of the port, not of BB2). | `ab3_cli`, `ab3_cli_ref` |
| #1069 | runtheprog (main.c: `runtheprog`), setint2 (stmts.c: `setint2`) | Two programs with runtime error checking or `SetErr` run side by side; code with runtime error checking that runs in supervisor mode outside a `SetInt` routine hangs at its first trap, which the original took. |  |
| #1070 | consttype2 `.quick2` (libcall.c:424, blitz2.s:10961); fetchdata, getelement2, negate (libcall.c:74, eval.c:1632, :1567, blitz2.s:10686, :9718, :9667) | A Quick literal just below 32768, such as `q=32767.9999`, is the largest Quick number; the original wrapped it to -32768 and `Data.q` refused it. | `compiler-quick-literal-wrap`, `compiler-quick-literal-data` |
| #1074 | dodata `.notb` (tokens1.c:707, blitz2.s:12666) | `Data.q -32768` stores the smallest Quick number; the original stopped the compile with "Bad Data". | `compiler-data-quick-min`, `compiler-data-quick-bounds`, `compiler-data-quick-over` |
| #1084 | typemode (tokens2.c:864, blitz2.s:13819) | An array field of a NewType with 65536 or more elements, or a negative count, is "Type too Big"; the original gave it the size of the count modulo 65536, and programs wrote past it. | `compiler-newtype-field-count-word`, `compiler-newtype-field-count-negative` |
| #1175 | loadalib (asmlibs.c `loadalib`, blitz2.s:6508-6512; doallocmem, blitz2.s:3379) | A library file whose relocation data holds no entries is loaded as a library without relocations; the original took it as out of memory and stopped at its start, or crashed under the editor. | `compiler-loadalib-reloc-count-0` |

## 2. Other deviations (kinds B, T, K)

Rows 1 to 14 are the rows of `../libs/deviations.md` section 5 (line
numbers as there); the rows after them are the further `DIFF:` notes that
are deliberate and documented elsewhere.

| Place (file:line) | Original | C version | Kind | observable? |
|---|---|---|---|---|
| errstack/errcont (asmlibs.c:1427, compile.c:184, data.c:1460, data.c:1830, main.c:1354, parse.c:216, parse.c:305, stmts.c:1189, stmts.c:1634) | error return is not reset | `errjmp = olderr` | **T** | stack trick (a7/errcont) only with longjmp in C; no |
| Stack tricks `addq #4,a7` / `move.l passstack,a7` (asmlibs.c:403, :467, asmlibs.h:69, compile.c:1108, tokens1.c:840, tokens1.c:953) | return past the caller | return value / longjmp | **T** | no |
| Placeholder longs (compile.c:829, stmts.c:432, tokens1.c:256, :545, :673, :1158, tokens2.c:10) | poke register leftovers as placeholders | 0 or the recognisable value | **T** | are always overwritten (except nomemleft); no |
| Register results (eval.h:236, eval.c:126, libcall.h:19, compile.c:1385, tokens1.c:343, tokens2.c:81, parse.c:1163) | results in d2/d3/d6/CCR, values from the caller's stack | globals / parameters / return values | **T** | no |
| Constant folding (eval.h:171, eval.c:824, libcall.c:1182) | executes generated code with the compiler registers | `callregs` with the relevant registers | **T** | no |
| Range/overflow checks (eval.c:486, data.c:1693, parse.c:351, compile.c:198) | without check (reading/writing beyond tables, DIVU by 0) | C checks and does nothing | **K** | only in cases that according to the comment do not occur |
| pushprec (eval.c:944) | stores operator/precedence without bounds check; "Expression too complex" only in `popprec`, after writing past the end of the stack | checks before writing and reports `pserr` immediately | **K** | same message; the only difference is that the original first writes beyond `precstackf` (only with very deeply nested expressions) |
| savefile HUNK_SYMBOL (parse.c:269) | writes the name length rounded up to longwords directly from the label node: **reads** up to 3 bytes behind the allocation and writes them as padding bytes into the executable | copy padded with zeros (padding bytes 0) | **K** | only with "Create Debug Info": the padding bytes in HUNK_SYMBOL (original random, C always 0); otherwise no |
| libcall.c:256, libcall.c:1438 | makeint/comparison with leftovers in the upper byte | FALSE / defined comparison | **T** | no |
| parse.c:544, parse.h:75, compile.h:32 | err without a set return | message + `exit(RETURN_FAIL)` | **T** | only with internal errors |
| parse.c:825 | `findlast` (dead code) | walks the list | **T** | no |
| stmts.c:1664 | flushc after the register restore | before it | **T** | no |
| Start (main.c:31, :1072, :1096, :1134, :1141) | own Workbench/CLI start, opens/closes dos.library | libnix start (DOSBase, WBStartup, argv) | **T** | command line is split by libnix (quotes) |
| main.c:1529 | SuperState/movec/UserState | `Supervisor()` | **T** | no |
| asmit short branches (asmlibs.c, ext_bb2.c, ab3syn.c `ext_asmshortbra`, asmmac) | `.s` is a Syntax Error; every `Bcc`, `BRA`, `BSR` is a word branch | extension #1075 (user decision 2026-11-16), BB2 mode: `.s` is `.b`; `Bcc`, `BRA`, `BSR` with `.s` or `.b` are short branches, with `.w` word branches; AB3 mode keeps AmiBlitz3's word branches | **B** | sources with `.s` compile now, a source with `Bcc.b` gets the short branch (tests `blitz2_asm_short_branch`, `_asm_short_range_fwd`, `_asm_short_range_back`, `_asm_short_zero`; `ab3_cli` cases `asm_short_s_bb2`, `asm_short_range_bb2`, `asm_short_zero_bb2`, `asm_bra_l_bb2`) |
| `blitz2version` (main.h:36) | `version` 195 in `blitz2.s` | 210, as in the compiler of the CD | **B** | the version in the requester; the compiler of the CD is the reference ([../v2.10/blitz2.md](../v2.10/blitz2.md)) |
| `loadblitzlibs` (asmlibs.c, `DIFF:` at the `'/'` test) | tests `':'` behind the libs directory, which gives `blitz2:userlibs//dir` | tests `'/'` as the compiler of the CD does (issue #2) | **B** | the name of the user libs directory |
| `selpushb` (tokens1.c:64) | `and #255,d0` as the reference build (vasm) encodes it (`$C07C`) | the encoding of Devpac (`andi`, `$0240`), as the compiler of the CD | **B** | no: the programs are byte-identical to those of 2.10 |
| `main` (main.c:638, #1080, a bug of the port, not of BB2) | the compiler runs on the stack it is started with | a stack of its own (`COMPILERSTACK`, 48 KB) when that one is smaller: the C evaluator needs about 270 bytes per parenthesis and 490 per function call | **T** | no: nesting that the original's tables allow no longer overflows the stack |
| `runtheprog` (main.c:1036) | the new process runs `runtheprog` as is | `__saveds`: with `-fbaserel` the process needs a4 = the base of the globals (#338) | **T** | no |
| Workbench start with a project icon (main.c:603, `BUG(orig)`, #21) | the old current directory lock is dropped and the directory is not restored when blitz2 ends | the code still drops the lock; libnix's startup code restores the directory it saved before `main`, so the port ends in the start directory | **T** | the current directory after the end of a Workbench start |
| PANIC (main.c:1085, `BUG(orig)`, #24) | removes the program process with `RemTask`, no cleanup | unchanged on purpose: a clean abort of a running program is not worth the complexity (not a deviation) | - | - |
