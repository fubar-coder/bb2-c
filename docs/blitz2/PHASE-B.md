# Phase B – go

All 10 headers exist and `make headers` passes. Now translate your chunk into
src/NAME.c following CONVENTIONS.md.

1. Read ALL headers in src/ (they are the contract) and notes/changes.md.
2. Translate EVERY routine and data label of your line range – no stubs, no
   "TODO: translate". If something truly cannot be done, implement the closest
   correct behaviour, mark it `DIFF:` and report it.
3. Build: `make build/NAME.o` – no errors; fix reasonable warnings.
   Also re-run `make headers` if you touched a header.
4. Keep the asm semantics including quirks (BUG(orig)). Line references
   `/* blitz2.s:NNNN */` on every function.

Cross-chunk facts settled in phase A (check the headers for details):
- Renames: `eos_` (asm eos, chunk 09; atokens entry!), `nop_` (09), `free_`
  (08), `getchar_` (05), `errjmp_` (asm errjmp global, chunk 03; `errjmp` is
  the jmp_buf pointer of parse.h).
- `firsttype` is `#define firsttype (stringtype.next)` (data.h).
- `struct asment` = asm mnemonic data (asmentry is an alias); asmpnts[] in
  chunk 03; checkers `asmchkfn` in chunk 01; getea API in stmts.h (chunk 03).
- Number fetch: `numd3`/`numd6` globals (libcall.h) carry asm d3/d6;
  fetchnum2 returns the value when `dfetch` is set, else the next char.
- `dolineabort` flag (compile.h) for handlers that drop doline's return
  (domacro); `passjmp` (compile.h) for doinc2's jump back into the pass loop.
- Editor: ed_*() wrappers + CD_L/CD_W (main.h), pointeron()/sleepon() keep
  their names, doallocmem() in stmts.h.
- Template arrays: owners define them; others use the externs/#defines.

If you need a declaration another header lacks, add it there marked
`/* added by chunk NN */` and log it in notes/changes.md (one line). If you
find that another chunk's header contract is wrong for your caller, do not
change its semantics silently – log it in notes/changes.md under "ISSUE:".

When done, reply with: build status, number of BUG(orig)/DIFF markers,
declarations you added to other headers, ISSUEs, anything left incomplete.
