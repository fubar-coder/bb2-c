; mathtranslib-init-endless (#202) - the init of mathtranslib opens
; mathtrans.library with OldOpenLibrary until it succeeds: if the library
; cannot be opened, a program that uses a transcendental function hangs
; at startup, before its first statement.
; The program runs through inject with the open failing (runwrap=, inject
; openlibfail=mathtrans.library) and uses Sin in a part it never runs,
; between two lines of output; at its end the finit of mathtranslib runs
; with base 0.
; Correct: the program runs and ends normally (the base is 0, Sin is not
; called).  The original hangs before the first line (the session ends by
; its time limit).
; @fixed #202
; @configs clib
; @pass runwrap=@INJECT@|openlibfail=mathtrans.library
; @pass timeout=30
NPrint "start"
a.f=1
Goto skip
a=Sin(a)
skip:
NPrint "end"
End
