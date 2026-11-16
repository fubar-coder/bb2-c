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

## The bucket state machine (task 2.1)

This section fixes the details of Decisions 1 and 2. Where it differs from
the layout sketched there (a `prev` pointer and a used count), this is the
one implemented: the stack order makes both unnecessary.

**Fields.**
- A bucket is `BUCKET_SIZE` = 4 + 8·k bytes (one AllocMem, MEMF_PUBLIC):
  - 00.l `up`: the empty spare bucket above this one, or 0;
  - 04... k entries of 8 bytes, each a list header `{next, first}` as
    before.
- `firstdata` (as before): the newest header, an entry of a bucket.
- `bklast` (new): the address of the last entry of firstdata's bucket; 0
  when there is no list.

**Invariants** (from the LIFO order, `analyse-memlib-buckets.md`):
- The used entries of a bucket are its first ones.
- Every bucket below firstdata's bucket is full.
- The `next` of an entry that is not the first of its bucket is the entry
  right below it (entry - 8). The `next` of a bucket's first entry is the
  last entry of the bucket below, or 0 for the global list. That is never
  entry - 8, because the 4 bytes at entry - 4 are the bucket's `up`.
- At most one empty bucket exists after any newmem or freelast: the `up`
  of the bucket just below it.

**newmem** (keeps d0-d7/a2-a6):
- Fast path: `firstdata != bklast`, so the bucket has a free entry. The
  new header is firstdata + 8: `next` = firstdata, `first` = 0, and
  firstdata points to it. Only a0/a1 are used, with no saves and no exec.
- Otherwise the bucket is full, or there is no list. d0-d1/a6 are saved.
  - The bucket to use is the full bucket's `up` (the spare), if there is
    one.
  - Otherwise a new bucket is allocated: its `up` = 0, and the full
    bucket's `up` = the new one (the full bucket is at bklast - (8k - 4)).
  - Its first entry becomes the header, and bklast = bucket + 8k - 4.
  - If AllocMem fails: err1, as today.

**freelast** (keeps d0/d2-d7/a2-a6, Z = 1 when there is no list):
- The header is unlinked as before (firstdata = its `next`). Then
  `next == entry - 8` is tested.
- Fast path (equal): the entry stays in its bucket as the next free one.
  Nothing else changes: no exec call for the header.
- Otherwise the entry was the first of its bucket, and that bucket is now
  empty:
  - bklast = the new firstdata (the last entry of the full bucket below,
    or 0);
  - the bucket's spare `up`, if any, is freed and `up` cleared. The last
    two buckets were empty: the user's hysteresis;
  - if the new firstdata is 0, the last list is gone and the bucket itself
    is freed too. "No list" therefore always means "no bucket", and the
    existing case "freelast ... all memory back" of
    `libs/tests/systest.c` stays true;
  - otherwise the bucket stays as the spare of the one below, whose `up`
    already points to it.
- Then the blocks are freed as before.

**finit, $C006, err1:** unchanged code. codefinit calls freelast until
firstdata = 0, and the last of those calls frees the last bucket. err1
calls codefinit. A failed bucket allocation in newmem happens before
anything is linked, so err1 finds a consistent chain. codeinit sets bklast
= 0 with firstdata = 0, and its newmem allocates the first bucket.

**Transitions at a boundary:**
- depth d → d+1, where d+1 is the first entry of a new bucket: the first
  time an AllocMem;
- back to d: that bucket becomes the spare;
- across the boundary again: the spare is used and kept, with no exec
  call;
- deeper back down: the spare of a bucket that becomes empty is freed. So
  a recursion of depth n allocates at most ceil((n+1)/k) buckets, the
  global list included.

**Static 68000 cycles** (without exec; the caller's JSR not counted, as in
the analysis; PC-relative reads of firstdata and bklast):

| Path | Today | Buckets |
|---|---:|---:|
| newmem, fast path | 214 (+ AllocMem) | **122** |
| newmem, a full bucket with a spare | 214 (+ AllocMem) | 282 |
| newmem, a new bucket | 214 (+ AllocMem) | 406 (+ AllocMem, once per k levels) |
| freelast, the header part (up to the block loop), fast path | 146 (+ FreeMem 8) | **142** |
| freelast, header + 2 blocks (bproc's path) | 360 | **356** |
| freelast, a bucket became empty, no spare above | 146 (+ FreeMem) | 220 |

Both fast paths are no slower than today without exec, and each of them
also saves the exec call (about 1890 measured cycles per pair).

## Bucket size (task 4.2)

Measured on main 5dd3347a + the buckets, two sessions each (perf_bproc,
perf_brec):

| Loop | Without buckets | k = 4 | k = 8 | Original |
|---|---:|---:|---:|---:|
| bproc.pstr | 9600.6 | 8185.0, 8184.6 | **8037.2, 8037.1** | 10038.3 |
| bproc.fstr | 10290.1 | 8875.0, 8875.0 | **8730.7, 8729.1** | 10783.6 |
| bproc.pgrow | 12915.1 | 11612.5, 11613.0 | **11087.2, 11089.1** | 13322.6 |
| brec.rec20 | 6230.7, 6230.2 | 5855.0, 5856.9 | **5327.8, 5328.3** | 6534.8 |

- brec's recursion (22 headers with the global list) needs 6 buckets at
  k = 4 and 3 at k = 8. Every repetition allocates 4 or 1 of them again,
  since only one spare stays.
- bproc stays at depth 1 and makes no bucket call in the loop. Its
  difference between k = 4 and k = 8 is exec's free list: the 36- or
  68-byte bucket lies elsewhere among the string blocks.
- k = 8 is better on every line and is kept. At most one empty bucket of
  68 bytes (72 with exec's rounding) stays allocated, within the 80 bytes
  of the proposal.
