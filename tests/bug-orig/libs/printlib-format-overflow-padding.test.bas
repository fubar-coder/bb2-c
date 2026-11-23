; printlib-format-overflow-padding (#238) - a number with more integer
; digits than the format has places gives "?"; the overflow branch skips
; the excess digits but then counts every place as padding, so only
; spaces follow (1234.5 with "##" gives "?  ").
; Here Str$ and NPrint of 1234.5 and 1234 with "##", of 12.25 with "#.#".
; Correct: "?" and the lowest digits, rounded: "?35" (1234.5), "?34"
; (1234 with "##"), "?2.3" (12.25 with "#.#").
DEFTYPE .l

!TestCase{padding}
  f.f=1234.5 : n=1234
  Format "##" : a$=Str$(f) : b$=Str$(n) : NPrint "a <",f,"> <",1234,">"
  f=12.25 : Format "#.#" : c$=Str$(f) : NPrint "b <",f,">"
  Format ""
  !CheckStrEq{a$,"?35","float"}
  !CheckStrEq{b$,"?34","long"}
  !CheckStrEq{c$,"?2.3","fraction"}
!EndTestCase
