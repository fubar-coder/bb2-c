# Design

## Context

- **memlib keeps a singly linked chain of list headers** (`firstdata`), 8 bytes each: `next` and the first memory block (`libs/acidlibs/memlib.c`).
  - The first header is always the current local list. The last one is the global list, which init creates with `newmem`.
  - `globalloc`/`globfree` find the global list by walking the chain (`findglob`).
  - `newmem` (assembler entry) allocates a header with exec AllocMem(8) and puts it in front.
  - `freelast` (assembler entry) takes `firstdata`, frees the header and every block of it with FreeMem, and makes `next` the current list.
  - Token $C006 and the library finit call `freelast` until the chain is empty.
- **Frees are strictly LIFO.** `freelast` always frees the newest header. When a runtime error, SetErr, `Goto` out of a procedure or the debugger leaves a procedure without its exit, its list stays at the front of the chain, and the next `freelast` (the outer procedure's) frees that one instead. This is today's behaviour, and the original's. The chain is therefore always a stack, and the headers are taken and given back in stack order.
- **Contracts:**
  - `newmem` keeps d0-d7/a2-a6: the parameters are in d0-d5 at procedure entry.
  - `freelast` keeps d0 (a Function's result) and d2-d7/a2-a6, and returns Z = 1 when there was no list.
  - Recorded KEEPS cases are in `libs/tests/systest.c`.
- **Timing:** static counts are newmem 214 and freelast 360 cycles without exec. One AllocMem/FreeMem pair costs about 1890 cycles and depends on the free list.

## Goals / Non-Goals

**Goals:**
- No exec call for a header while the depth stays within the buckets already allocated.
- Memory given back when the depth goes down, with a small fixed hysteresis.
- newmem and freelast are not slower than today on the path without exec, and well below the original with exec saved.

**Non-Goals:**
- The memory blocks of the lists: strings, arrays, localloc/globalloc. A block cache is a separate decision.
- Changing the LIFO behaviour of `freelast` for abandoned frames; that is the original's design.
- AB3 mode, which uses AmiBlitz3's BigLibs.

## Decisions

1. **A stack of buckets, not a free mask.**
   - Since headers are taken and freed in stack order, a bucket needs only a count of used entries; the newest bucket is the top.
   - Layout of a bucket: `prev` bucket pointer, used count (word), k entries of 8 bytes.
   - memlib keeps a pointer to the top bucket.
   - The header address in the chain stays the entry's address, so `findglob`, `globalloc`, `localloc` and the list layout do not change.
   - *Alternative:* a per-entry used mask, which is needed only if frees could be out of order; the analysis confirms they cannot.
2. **Freeing rule (the user's hysteresis).**
   - When `freelast` gives the last entry of the top bucket back, that bucket is empty.
   - If the bucket below it is empty too, which can only be when it was emptied before and the top bucket was kept as spare, the top bucket goes back to exec. The top pointer moves down.
   - The result: at most two empty buckets exist at any time, and a call/return across a bucket boundary costs no exec call after the first.
   - The exact state machine (where "empty top bucket kept as spare" lives) is fixed in task 2.1 and checked by the library cases.
3. **Bucket size.**
   - Start with k = 4 (a bucket of 4 + 4·8 = 36 bytes, or 40 bytes with alignment).
   - The analysis measures k = 4 and k = 8 on `bproc` and on a recursion line, then picks one; the value is a constant in memlib.
4. **Where the global list lives.** The global list is the first header, from init. It stays in the first bucket and is freed only by finit.
5. **Assembler entries.** `newmem` and `freelast` stay assembler entries with the same saved registers as today. The bucket test is a few instructions on the fast path: compare the count with k, then index. Nothing is added to `localloc`/`locfree`.
6. **Out of memory.**
   - If a new bucket cannot be allocated, `newmem` raises "Out of Memory!" as today, through err1.
   - A kept spare bucket is always used before a new one is allocated, so an allocation fails only when no empty bucket exists. Flushing buckets before "Out of Memory!" would therefore free nothing, and no flush step is needed for headers.
   - The analysis checks whether err1's "free all" path handles the buckets.

## Risks / Trade-offs

- **[Another library or a CD program walks or frees list headers itself.]** → Task 1.1 searches every caller of the memlib table and of `firstdata` (acidlibs, RWE userlibs, the CD's third-party libraries, AB3 libraries with inline assembler). If one frees a header with FreeMem itself, the change stops and reports.
- **[memlib is reached from interrupt code (SetInt).]** → Task 1.1 confirms that the tokens are not interrupt-safe today either, since the chain update is not atomic. Then the buckets add no new risk.
- **[Up to about 80 bytes stay allocated while a program runs]**, and AvailMem shows them as used. → This is the accepted trade-off (kind B row in deviations.md).
- **[A bug in the bucket bookkeeping loses or doubles memory.]** → Library cases for every transition (first entry, full bucket, new bucket, spare kept, spare freed, finit with open frames), and a leak test with inject's allocation counts at program end.

## Migration Plan

- This change needs the separate memlib rework (proposal A: localloc/locfree/globalloc/globfree as assembler entries) merged first, because both change memlib.
- Rollback is the revert of the change's commits; no data format outside the running program changes.
