; printlib-format-no-rounding (#236) - Format rounds only if d3 is not 0,
; but d3 is then -(the number of integer digits), not the number of the
; format's digit places: a number without an integer digit (|x| < 1) is
; never rounded, only cut: 0.567 with ".##" gives ".56".
; Here Str$ and NPrint of 0.567 with ".##", "###.##", "0.0" and of -0.567
; with "-###.#".
; Correct: rounded like a number with integer digits: ".57", "   .57"
; ("###.##"), "0.6" ("0.0"), "-   .6" ("-###.#" with -0.567).
DEFTYPE .l

!TestCase{no_rounding}
  f.f=0.567
  Format ".##" : a$=Str$(f) : NPrint "a <",f,">"
  Format "###.##" : b$=Str$(f) : NPrint "b <",f,">"
  Format "0.0" : c$=Str$(f) : NPrint "c <",f,">"
  f=-0.567 : Format "-###.#" : d$=Str$(f) : NPrint "d <",f,">"
  Format ""
  !CheckStrEq{a$,".57","a"}
  !CheckStrEq{b$,"   .57","b"}
  !CheckStrEq{c$,"0.6","c"}
  !CheckStrEq{d$,"-   .6","d"}
!EndTestCase
