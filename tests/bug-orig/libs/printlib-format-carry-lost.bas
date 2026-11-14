; printlib-format-carry-lost (#235) - Format rounds at the last decimal by
; incrementing the digits already written, from the last one backwards; a
; carry out of the first digit is lost and the number turns into zeros:
; 999.96 with "####.#" gives " 000.0".
; Correct: the carry makes a new leading digit: "1000.0" ("####.#"),
; "10.00" ("##.##" with 9.996), "-1000.0" ("-####.#" with -999.96).
; @fixed #235
; @configs clib
Format "####.#" : f.f=999.96 : NPrint "a <",f,">"
Format "##.##" : f=9.996 : NPrint "b <",f,">"
Format "-####.#" : f=-999.96 : NPrint "c <",f,">"
Format ""
End
