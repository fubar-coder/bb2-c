; mathtranslib-init-endless (#202) - the init of mathtranslib opens
; mathtrans.library with OldOpenLibrary until it succeeds: if the library
; cannot be opened, a program that uses a transcendental function hangs
; at startup, before its first statement.
; The program runs through inject with the open failing (runwrap=, inject
; openlibfail=mathtrans.library) and uses Sin in a part it never runs,
; between two checks; at its end the finit of mathtranslib runs with
; base 0.
; Correct: the program runs and ends normally (the base is 0, Sin is not
; called).  The original hangs before the first check (the session ends
; by its time limit).
DEFTYPE .l

!TestCase{sin_not_called}
  !Check{-1,"start"}
  a.f=1
  Goto skip
  a=Sin(a)
.skip
  !Check{-1,"end"}
!EndTestCase
