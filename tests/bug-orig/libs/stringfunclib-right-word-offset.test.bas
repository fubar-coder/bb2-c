; stringfunclib-right-word-offset (#285) - Right$(s$,n) forms the address
; of its first character with LEA 0(a3,d0.w),a0: the offset Len(s$)-n is
; a sign extended word, so for Len(s$)-n >= 32768 the characters come from
; before the string (foreign memory).  UnRight$ (Right$ with Len(s$)-n)
; shares the code.  A string of 40003 characters ending in "xyz"; the
; string buffer is raised to 150000 bytes (tedstub strbuf=) so that it
; fits.
; Correct: Right$(a$,3) and UnRight$(a$,40000) are "xyz".
DEFTYPE .l

!TestCase{word_offset}
  a$=String$("a",40000)+"xyz"
  r$=Right$(a$,3)
  u$=UnRight$(a$,40000)
  !CheckStrEq{r$,"xyz","right"}
  !CheckEq{Len(r$),3,"right_len"}
  !CheckStrEq{u$,"xyz","unright"}
!EndTestCase
