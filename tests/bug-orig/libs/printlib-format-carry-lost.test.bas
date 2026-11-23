; printlib-format-carry-lost (#235) - Format rounds at the last decimal by
; incrementing the digits already written, from the last one backwards; a
; carry out of the first digit is lost and the number turns into zeros:
; 999.96 with "####.#" gives " 000.0".
; Here Str$ and NPrint with the format on 999.96 ("####.#"), 9.996
; ("##.##") and -999.96 ("-####.#").
; Correct: the carry makes a new leading digit: "1000.0" ("####.#"),
; "10.00" ("##.##" with 9.996), "-1000.0" ("-####.#" with -999.96).
DEFTYPE .l

!TestCase{carry_lost}
  Format "####.#" : f.f=999.96 : a$=Str$(f) : NPrint "a <",f,">"
  Format "##.##" : f=9.996 : b$=Str$(f) : NPrint "b <",f,">"
  Format "-####.#" : f=-999.96 : c$=Str$(f) : NPrint "c <",f,">"
  Format ""
  !CheckStrEq{a$,"1000.0","a"}
  !CheckStrEq{b$,"10.00","b"}
  !CheckStrEq{c$,"-1000.0","c"}
!EndTestCase
