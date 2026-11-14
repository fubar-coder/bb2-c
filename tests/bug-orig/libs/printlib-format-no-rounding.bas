; printlib-format-no-rounding (#236) - Format rounds only if d3 is not 0,
; but d3 is then -(the number of integer digits), not the number of the
; format's digit places: a number without an integer digit (|x| < 1) is
; never rounded, only cut: 0.567 with ".##" gives ".56".
; Correct: rounded like a number with integer digits: ".57", "   .57"
; ("###.##"), "0.6" ("0.0"), "-   .6" ("-###.#" with -0.567).
; @fixed #236
; @configs clib
f.f=0.567
Format ".##" : NPrint "a <",f,">"
Format "###.##" : NPrint "b <",f,">"
Format "0.0" : NPrint "c <",f,">"
f=-0.567 : Format "-###.#" : NPrint "d <",f,">"
Format ""
End
