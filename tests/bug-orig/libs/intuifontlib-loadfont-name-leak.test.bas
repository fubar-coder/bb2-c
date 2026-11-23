; intuifontlib-loadfont-name-leak (#198) - LoadFont copies the font name
; (maxslib allocst) into the object and never frees the copy of a
; previous LoadFont on the same object: every LoadFont costs the memory
; of a name copy until the program ends (memlib frees it then).
; One LoadFont first (diskfont's own allocations), then the free memory
; (exec AvailMem) before and after 50 more LoadFont on the same object,
; under Forbid; topaz 8 is a ROM font.  50 copies of "topaz.font" are at
; least 750 bytes.
; Correct: no memory lost (less than 200 bytes).
DEFTYPE .l

!TestCase{name_copies}
  LoadFont 0,"topaz.font",8
  MOVE.l 4,a6 : JSR -132(a6)
  MOVEQ #0,d1 : MOVE.l 4,a6 : JSR -216(a6) : PutReg d0,m1
  For i=1 To 50
    LoadFont 0,"topaz.font",8
  Next
  MOVEQ #0,d1 : MOVE.l 4,a6 : JSR -216(a6) : PutReg d0,m2
  MOVE.l 4,a6 : JSR -138(a6)
  !Check{(m1-m2)<200,"lost_under_200"}
!EndTestCase
