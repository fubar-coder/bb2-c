; blitzcoplib-marker-customcop (#405) - Slice marks the place of a line
; 256 wait with the copper move $01FE1234; the copy loop of inscop2 looks
; for this long in every inserted block, also in the string of CustomCop:
; it replaces it by $01FE0000 or the line 255 wait (blwait) and turns the
; following word of the source into a wait (low byte, 1).
; Here: CustomCop at line 100 with the string $01FE1234 $01800F00 (a MOVE
; to the unused register $1FE, a colour move), in Amiga mode; the program
; reads its string variable and the copper list (CopLoc, CopLen).  The
; string variable is not changed (CustomCop gets a copy in the string
; work buffer, which is changed); the original puts blwait and the wait
; $80010F00 into the list instead of the two moves.
; Correct: the list holds the instructions as given.
DEFTYPE .l

!TestCase{marker_in_string}
  c$=Mki$($1FE)+Mki$($1234)+Mki$($180)+Mki$($F00)
  CustomCop c$,100
  !CheckEq{Peek.l(&c$),$01FE1234,"string_0"}
  !CheckEq{Peek.l(&c$+4),$01800F00,"string_1"}
  l=CopLoc : n=CopLen
  !CheckEq{n,20,"len"}
  !CheckEq{Peek.l(l),$00960020,"list_0"}
  !CheckEq{Peek.l(l+4),$6407FFFE,"list_1"}
  !CheckEq{Peek.l(l+8),$01FE1234,"list_2"}
  !CheckEq{Peek.l(l+12),$01800F00,"list_3"}
  !CheckEq{Peek.l(l+16),$FFFFFFFE,"list_4"}
!EndTestCase
