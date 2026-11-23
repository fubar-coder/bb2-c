; wbstartuplib-clistring-overflow (#308) - started from the CLI, the
; argument line is copied into cliString (256 bytes) without a length
; check: a longer line overwrites what is behind it (the libraries linked
; after wbstartuplib) and the program crashes at its start.
; The program is started with an argument of 306 characters (runargs;
; the shell refuses a command line of 1000 characters with "Command too
; long" before the program starts, so the argument stays below that).
; Correct: the argument line is cut to the 255 characters that fit, the
; program gets them as its argument and ends normally.
DEFTYPE .l

!TestCase{long_argument}
  n=NumPars
  a$=Par$(1)
  l=Len(a$)
  s$=Left$(a$,3)
  e$=Right$(a$,3)
  !CheckEq{n,1,"numpars"}
  !CheckEq{l,255,"length"}
  !CheckStrEq{s$,"abc","start"}
  !CheckStrEq{e$,"xxx","end"}
!EndTestCase
