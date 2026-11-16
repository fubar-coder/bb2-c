# Proposal

## Why

Every call of a procedure that has local strings or other local memory goes through memlib's `newmem` at entry and `freelast` at exit. Each of them makes an exec call for the 8-byte list header (AllocMem, then FreeMem), at about 1890 cycles per pair, the biggest single cost of such a call (the `bproc.pstr` analysis, `~/.local/share/bb2-c-work/analyse-pstr-fstr.md`: 56 % of the call is exec, one of the three pairs is the header). A prototype that reuses headers measured `bproc.pstr` 9600 -> 7818 cycles. The user asked for a form that also gives the memory back when the recursion depth goes down: headers in buckets.

## What Changes

- memlib keeps the list headers in buckets of a fixed number of entries (4 to start with; the analysis measures 4 and 8). `newmem` takes the next free entry. Only when every bucket is full does it allocate a new bucket from exec.
- `freelast` gives the top entry back. When the last two buckets are empty, the last one goes back to exec (hysteresis, so a program that calls and returns at a bucket boundary does not allocate and free each time).
- The library finit (and token $C006, "free ALL memory") frees all buckets as well as the blocks.
- The memory blocks of the lists (strings, arrays) are not part of this change. They keep one AllocMem/FreeMem each. A block cache is a separate later decision.
- No behaviour of a Blitz program changes except when memory goes back to the system: up to two empty buckets (about 80 bytes) stay allocated while the program runs. This is an intended deviation of kind B (user decision).

## Capabilities

### New Capabilities

- `libs/local-memory`: how memlib provides and returns the memory of procedure-local lists. That covers when exec is called, what stays allocated, and what the end of the program frees.

### Modified Capabilities

(none; the register contracts of `libs/register-contract` stay as they are)

## Impact

- `libs/acidlibs/memlib.c`: `newmem`, `freelast`, codeinit/codefinit, and the data of the bucket list. All of these are already assembler entries or small C functions; `localloc`/`locfree`/`globalloc`/`globfree` are not changed by this change. They become assembler entries in a separate rework, proposal A of the analysis, which must be merged first.
- Tests:
  - library cases for newmem/freelast (`libs/tests/systest.c`, KEEPS unchanged);
  - a leak test that the end of the program frees everything, also after a runtime error inside a recursion;
  - perf lines (`bproc`, a recursive procedure).
- `docs/libs/deviations.md` (kind B row), `docs/libs/cmdlibs-port.md`.
- Every program that links memlib (nearly all) gets a different executable; outputs stay unchanged.
- AB3 mode uses AmiBlitz3's own BigLibs and is not affected (to be confirmed by the analysis).
