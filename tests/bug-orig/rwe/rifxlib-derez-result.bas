; rifxlib-derez-result (#672) - Derez is declared with a long result
; (!acommand {#long}) but never sets one: d0 is what the routine leaves
; there - its d0 on entry for a derez value of 0 (64 here), the upper
; word of it and $FFFF for 1, the last long of UnrezX2 for 2, a mask or
; source word for the others.  Bitmaps 1 and 2 are lores (320 x 16, one
; plane, cleared).
; Correct: Derez returns 0 when it does nothing (derez value 0) and -1
; when it worked (W6 decision Q3).
; @fixed #672
; @rwe rifxlib
DEFTYPE .l
BitMap 1,320,16,1
BitMap 2,320,16,1
r=Derez(1,2,0,0,0,8) : NPrint "0: ",r
r=Derez(1,2,0,0,1,8) : NPrint "1: ",r
r=Derez(1,2,0,0,2,8) : NPrint "2: ",r
r=Derez(1,2,0,0,3,8) : NPrint "3: ",r
End
