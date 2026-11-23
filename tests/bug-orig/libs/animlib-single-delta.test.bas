; animlib-single-delta (#401) - after applying a delta NextFrame goes to
; the next node; at the end of the list it wraps to the node AFTER the
; first one.  With a single DLTA that link is 0: the current delta (+26)
; becomes 0 and the next NextFrame reads its delta node at address 0.
; Here: an ANIM (generated) of one row $AABB with one DLTA that writes
; $CCDD; InitAnim, then NextFrame twice, the row and the current delta
; node (+26) after each.
; Correct: the anim plays once and stays on its last frame: after either
; NextFrame the row is $CCDD and the current delta is the first (only)
; node again.
DEFTYPE .l

!TestCase{single_delta}
  BitMap 0,16,1,1
  LoadAnim 0,"CORPUS:onedelta.anim"
  a=Addr Anim(0)
  !CheckEq{Frames(0),0,"frames"}
  !Check{Peek.l(a+18)<>0,"has_delta"}
  InitAnim 0
  !CheckEq{Peek.w(Peek.l(Addr BitMap(0)+8)) AND $FFFF,$AABB,"init_row"}
  !Check{Peek.l(a+26)=Peek.l(a+18),"init_current"}
  NextFrame 0
  !CheckEq{Peek.w(Peek.l(Addr BitMap(0)+8)) AND $FFFF,$CCDD,"row_1"}
  !Check{Peek.l(a+26)=Peek.l(a+18),"current_1"}
  NextFrame 0
  !CheckEq{Peek.w(Peek.l(Addr BitMap(0)+8)) AND $FFFF,$CCDD,"row_2"}
  !Check{Peek.l(a+26)=Peek.l(a+18),"current_2"}
!EndTestCase
