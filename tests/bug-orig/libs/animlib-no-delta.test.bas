; animlib-no-delta (#401) - NextFrame applies the current delta node
; without a check: an ANIM with an ANHD but no DLTA has no delta node
; (+18 = 0), InitAnim makes the current delta (+26) 0, and NextFrame
; reads its delta node at address 0: plane offsets from address 8 (the
; exception vectors), ops from wherever they point, written into the
; bitmap.
; Here: an ANIM (generated) of one row $AABB with an ANHD and no DLTA;
; InitAnim, NextFrame, then the row.
; Correct: without a delta NextFrame does nothing: the row stays $AABB.
DEFTYPE .l

!TestCase{no_delta}
  BitMap 0,16,1,1
  LoadAnim 0,"CORPUS:nodelta.anim"
  a=Addr Anim(0)
  !CheckEq{Frames(0),-1,"frames"}
  !Check{Peek.l(a+18)=0,"no_delta_node"}
  InitAnim 0
  !CheckEq{Peek.w(Peek.l(Addr BitMap(0)+8)) AND $FFFF,$AABB,"init_row"}
  NextFrame 0
  !CheckEq{Peek.w(Peek.l(Addr BitMap(0)+8)) AND $FFFF,$AABB,"row"}
!EndTestCase
