; ffplib-open-unchecked (#137; the issue names no bug ID) - the init of
; ffplib opens mathffp.library with OldOpenLibrary and takes the result
; unchecked as the base of every floating point operation.  The program
; runs through inject with the open failing (runwrap=, inject
; openlibfail=mathffp.library): the base is 0, the first multiplication
; jumps to a negative offset from 0 and the system crashes.
; Correct: the failed open is noticed - the program ends without a crash
; (how it ends, with or without an error, is open; only the session is
; checked).
; @configs clib
; @pass runwrap=@INJECT@|openlibfail=mathffp.library
; @pass timeout=45
a.f=2.5
b.f=a*3
NPrint b
End
