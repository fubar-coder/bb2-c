; rifxlib-derez-result (#672) - Derez is declared with a long result
; (!acommand {#long}) but never sets one: d0 is what the routine leaves
; there - its d0 on entry for a derez value of 0 (64 here), the upper
; word of it and $FFFF for 1, the last long of UnrezX2 for 2, a mask or
; source word for the others.  Bitmaps 1 and 2 are lores (320 x 16, one
; plane, cleared).
; Correct: Derez returns 0 when it does nothing (derez value 0) and -1
; when it worked (W6 decision Q3).
DEFTYPE .l

!TestCase{derez_value}
  BitMap 1,320,16,1
  BitMap 2,320,16,1
  r0=Derez(1,2,0,0,0,8)
  r1=Derez(1,2,0,0,1,8)
  r2=Derez(1,2,0,0,2,8)
  r3=Derez(1,2,0,0,3,8)
  !CheckEq{r0,0,"value_0"}
  !CheckEq{r1,-1,"value_1"}
  !CheckEq{r2,-1,"value_2"}
  !CheckEq{r3,-1,"value_3"}
!EndTestCase
