; risnowlib-activate-before-dosnow (#908) - InitSnowDrop leaves the
; drop list's "last" field (offset 12) at 0; only LDoSnow sets it.
; LDoSnowActivate appends its entries there and writes the end mark
; $ABCD: before the first LDoSnow it writes to address 0.  The kill list
; is empty, so only the end mark is written (the word at 0, restored).
; Correct: LDoSnowActivate leaves address 0 alone.
DEFTYPE .l

!TestCase{activate_before_dosnow}
  BitMap 0,320,256,3
  w.w=Peek.w(0)
  Poke.w 0,0
  lt=InitSnowDrop(0,500,0,10240)
  n=LDoSnowActivate(0)
  v=Peek.w(0) AND $FFFF
  Poke.w 0,w
  !CheckEq{v,0,"address_0"}
!EndTestCase
