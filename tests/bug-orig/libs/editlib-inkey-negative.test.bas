; editlib-inkey-negative (#1031) - Inkey$(n) counts its n attempts as an
; unsigned long (SUBQ.l #1,d7:BNE 'loop): a negative n is about 4 billion
; attempts to read a character, and every character that comes is
; appended to the string, past the end of the string buffer.  The
; original has no error check for it (a programming error), not even
; with runtime error checking.
; Here with runtime error checking: the input is a file of the two
; characters "xy" (FileInput: one character per attempt, none at the end
; of the file), then Inkey$(-1): the original reads "xy" and then makes
; its other attempts at the end of the file - it does not come back (the
; session times out).
; Correct: the error check refuses the negative count: "Can't Inkey$ a
; negative number of Characters!".
DEFTYPE .l

!TestCase{negative_count}
  ok=WriteFile(0,"CORPUS:editlib-inkey-negative")
  !Require{ok,"file_written"}
  FileOutput 0 : Print "xy" : CloseFile 0
  DefaultOutput
  ok=ReadFile(0,"CORPUS:editlib-inkey-negative")
  !Require{ok,"file_read"}
  FileInput 0
  !ExpectError{"Can't Inkey$ a negative number of Characters!","refused"}
  a$=Inkey$(-1)
  !Check{0,"not_refused"}
  DefaultInput
  CloseFile 0
!EndTestCase
