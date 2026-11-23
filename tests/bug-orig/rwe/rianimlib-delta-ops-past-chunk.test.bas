; rianimlib-delta-ops-past-chunk (#1120, the part kept) - the ops of a
; DLTA's columns are not bounded by the chunk: a column whose op count
; claims more ops than the DLTA holds reads them behind it.  32 x 16, 1
; plane: frame 2's column 0 claims 2 ops, the DLTA ends behind the first
; (skip 3 rows); the second, behind the chunk, copies $AA to row 3.
; Kept on purpose (user decision "C2 findings" of 2026-11-19): a check
; per op would cost every delta op of RINextAnimFrame (D-007).
; Correct: no op behind the DLTA is played (row 3 stays 0).
; RIPackLib calls RIAmosFuncLib.
DEFTYPE .l

!TestCase{ops_past_chunk}
  a=AllocMem_(4096,$10005)
  !Require{a<>0,"memory"}
  f=Open_("CORPUS:anim",1005)
  !Require{f<>0,"file"}
  n=Read_(f,a,4096)
  Close_ f
  BitMap 0,32,16,1
  r=RIAnimInit(a,0,0)
  p=Peek.l(Addr BitMap(0)+8)
  r=RINextAnimFrame(0)
  v=Peek.b(p+12) AND $FF
  FreeMem_ a,4096
  !CheckEq{v,0,"row_3"}
!EndTestCase
