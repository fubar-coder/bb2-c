; riamosfunclib-count-zero (#492) - CopyByte (like CopyWord, CopyLong and
; FillMem) tests the count only after the first unit (SUBQ.l #1 : BNE):
; a count of 0 copies 2^32 bytes over all memory from the destination on
; - the program does not come back (the session ends in a timeout).
; Made safe: at full speed the copy went round the whole address space
; in the time limit, through the custom chips and the expansion area,
; and sometimes Amiberry itself ended ("exited early").  Now the CPU
; runs at the speed of a real A1200 without the JIT (about 1 MB per
; second), both blocks are taken from the top of memory (MEMF_REVERSE:
; the copy runs into the unmapped space behind the Z3 RAM at once), and
; the copy runs under Disable: nothing else runs on the overwritten
; memory, and the end of the address space is not reached in the time
; limit.
; Correct: a count of 0 copies or fills nothing, the program goes on
; (CopyWord, CopyLong and FillMem behind CopyByte: the original never
; gets there).
DEFTYPE .l

!TestCase{count_zero}
  a=AllocMem_(16,$50001)
  b=AllocMem_(16,$50001)
  !Require{a<>0 AND b<>0,"memory"}
  Disable_
  CopyByte a,b,0
  CopyWord a,b,0
  CopyLong a,b,0
  FillMem b,0,1
  FillMem b,0
  Enable_
  !CheckEq{Peek.l(b),0,"b_unchanged"}
  FreeMem_ a,16 : FreeMem_ b,16
!EndTestCase
