; datelib-numdays-word-carry (#122) - NumDays ends with SUBQ.w #1 (and,
; for January and February of a year that is no leap year, ADDQ.w #1)
; on the day count: at a word boundary there is no borrow or carry into
; the high word, so 31/12/1977 (the day -1) gives 65535 and the day
; 65535 (in 2157) gives 131071.
; Here NumDays of dates around the first boundary and the round trip
; NumDays(Date$(d)) around the second.
; Correct: 31/12/1977 is -1, the round trip gives the day back.  The
; original gives 65535 and 131071.
DEFTYPE .l

!TestCase{word_carry}
  a=NumDays("30/12/1977")
  b=NumDays("31/12/1977")
  c=NumDays("1/1/1978")
  d1$=Date$(65534) : r1=NumDays(d1$)
  d2$=Date$(65535) : r2=NumDays(d2$)
  d3$=Date$(65536) : r3=NumDays(d3$)
  !CheckEq{a,-2,"before_boundary"}
  !CheckEq{b,-1,"boundary"}
  !CheckEq{c,0,"after_boundary"}
  !CheckEq{r1,65534,"round_65534"}
  !CheckEq{r2,65535,"round_65535"}
  !CheckEq{r3,65536,"round_65536"}
!EndTestCase
