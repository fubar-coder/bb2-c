; printlib-format-sign-lost (#237) - Format takes the "-" off the number
; and writes it only for a "-" or "+" of the format string: a negative
; number printed with a format without them appears positive (-7 with
; "###" gives "  7").
; Correct: the "-" stands in front of the number, in front of the zeros
; of "0" places: " -7" ("###"), " -2.5 " ("###.##"), "-07" ("000"),
; "   -7" ("#,###"); positive numbers as before.
; @fixed #237
; @configs clib
Format "###" : NPrint "a <",-7,"> <",7,">"
Format "###.##" : f.f=-2.5 : NPrint "b <",f,">"
Format "000" : NPrint "c <",-7,">"
Format "#,###" : NPrint "d <",-7,">"
Format ""
End
