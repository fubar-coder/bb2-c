; printlib-format-sign-lost (#237) - Format takes the "-" off the number
; and writes it only for a "-" or "+" of the format string: a negative
; number printed with a format without them appears positive (-7 with
; "###" gives "  7").
; Here Str$ and NPrint of -7 and 7 with "###", of -2.5 with "###.##", of
; -7 with "000" and "#,###".
; Correct: the "-" stands in front of the number, in front of the zeros
; of "0" places: " -7" ("###"), " -2.5 " ("###.##"), "-07" ("000"),
; "   -7" ("#,###"); positive numbers as before.
DEFTYPE .l

!TestCase{sign_lost}
  m=-7 : p=7
  Format "###" : a$=Str$(m) : b$=Str$(p) : NPrint "a <",-7,"> <",7,">"
  Format "###.##" : f.f=-2.5 : c$=Str$(f) : NPrint "b <",f,">"
  Format "000" : d$=Str$(m) : NPrint "c <",-7,">"
  Format "#,###" : e$=Str$(m) : NPrint "d <",-7,">"
  Format ""
  !CheckStrEq{a$," -7","minus"}
  !CheckStrEq{b$,"  7","plus"}
  !CheckStrEq{c$," -2.5 ","fraction"}
  !CheckStrEq{d$,"-07","zeros"}
  !CheckStrEq{e$,"   -7","comma"}
!EndTestCase
