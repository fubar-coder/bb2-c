; windowslib-qualifier-long (#313) - Qualifier is declared as a long
; function, but the routine only loads a word into d0 (MOVE.w, AND.w
; #255): the upper word of the result is what d0 held before the call.
; Here: d0 gets $12340000 right before the call; no key was read, so the
; qualifier is 0.
; Correct: Qualifier gives 0, the whole long.
DEFTYPE .l

!TestCase{upper_word}
  WbToScreen 0
  Window 0,20,30,200,100,$000E,"Qualifier",1,2
  MOVE.l #$12340000,d0
  q=Qualifier
  !CheckEq{q,0,"qualifier"}
  CloseWindow 0
!EndTestCase
