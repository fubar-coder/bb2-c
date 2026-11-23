; iffiolib-zero-count (#179) - iffread (and iffskip) count down with
; SUBQ.l/BNE and test the count only after the first byte: a count of 0
; runs until 2^32.  LoadSound passes the BODY length on: for an empty BODY
; it allocates 0 bytes (memlib: 8 bytes of header only) and iffread copies
; the rest of the file (here the 8 bytes of an empty ANNO chunk) behind
; that block.  So that this hits nothing foreign, the program first takes
; two blocks of 8 bytes of chip memory and frees the first one again:
; LoadSound's block takes its place, and the bytes behind it are the
; second block (cleared), which the program reads back.
; Correct: an empty BODY copies nothing - the second block stays 0.
DEFTYPE .l

!TestCase{empty_body}
  a=AllocMem_(8,2)
  b=AllocMem_(8,2)
  !Require{a<>0 AND b<>0,"blocks"}
  Poke.l b,0 : Poke.l b+4,0
  FreeMem_ a,8
  LoadSound 0,"CORPUS:empty.8svx"
  d=Peek.l(Addr Sound(0))
  !Check{d=b,"data_behind_first_block"}
  !CheckEq{Peek.l(b),0,"second_block_0"}
  !CheckEq{Peek.l(b+4),0,"second_block_1"}
  FreeMem_ b,8
!EndTestCase
