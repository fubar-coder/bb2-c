; riarexxlib-no-rexxsyslib (#1094) - init does not check whether
; rexxsyslib.library opened, and the commands that build a message call
; CreateArgString through the base unconditionally.  The program runs
; through inject with the library failing to open (openlibfail=) and
; sends a command to its own port with RXRunCommand and RXSendCommand.
; Correct: nothing is sent without the library - RXRunCommand 0,
; RXSendCommand ""; the original jumps through a base of 0 (a crash).
DEFTYPE .l

!TestCase{no_library}
  p=RXCreatePort("BUGORIG")
  created=(p<>0)
  !CheckEq{created,-1,"created"}
  r=RXRunCommand("BUGORIG","hello")
  !CheckEq{r,0,"run"}
  a$=RXSendCommand("BUGORIG","hello")
  !CheckStrEq{a$,"","send"}
!EndTestCase
