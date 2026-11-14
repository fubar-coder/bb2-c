; printlib-format-overflow-padding (#238) - a number with more integer
; digits than the format has places gives "?"; the overflow branch skips
; the excess digits but then counts every place as padding, so only
; spaces follow (1234.5 with "##" gives "?  ").
; Correct: "?" and the lowest digits, rounded: "?35" (1234.5), "?34"
; (1234 with "##"), "?2.3" (12.25 with "#.#").
; @fixed #238
; @configs clib
f.f=1234.5
Format "##" : NPrint "a <",f,"> <",1234,">"
f=12.25 : Format "#.#" : NPrint "b <",f,">"
Format ""
End
