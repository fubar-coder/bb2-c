; inputoutputlib-default-typo (#193) - the runtime error of Inkey$ from the
; default input (which has no single characters) reads "Can't Input
; Character from Defualt Input": a typo.  As the corpus test t43, with
; runtime error checking.
; Correct: "Can't Input Character from Default Input".
; @fixed #193
; @configs clib
; @pass runtime_errors=on
NPrint "before"
a$=Inkey$
NPrint "after [",a$,"]"
End
