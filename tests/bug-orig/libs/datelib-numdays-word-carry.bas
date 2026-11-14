; datelib-numdays-word-carry (#122) - NumDays ends with SUBQ.w #1 (and,
; for January and February of a year that is no leap year, ADDQ.w #1)
; on the day count: at a word boundary there is no borrow or carry into
; the high word, so 31/12/1977 (the day -1) gives 65535 and the day
; 65535 (in 2157) gives 131071.
; Here NumDays of dates around the first boundary and the round trip
; NumDays(Date$(d)) around the second.
; Correct: 31/12/1977 is -1, the round trip gives the day back.  The
; original gives 65535 and 131071.
; @fixed #122
; @configs clib
DEFTYPE .l
NPrint "30/12/1977 ",NumDays("30/12/1977")
NPrint "31/12/1977 ",NumDays("31/12/1977")
NPrint "1/1/1978 ",NumDays("1/1/1978")
For d=65534 To 65536
  NPrint d," ",NumDays(Date$(d))
Next
End
