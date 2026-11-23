; blitzkeyslib-off-handler (#408) - BlitzKeys On installs BlitzKey as
; the input handler (the old one in oldin), BlitzKeys Off only clears the
; flag: the handler stays.  A second BlitzKeys On installs it again and
; oldin becomes BlitzKey itself, so reading a line (the handler's -4
; entry jumps to -4 of oldin) jumps to itself for ever.
; Here: a file in CORPUS: is the input (FileInput), BlitzKeys On, Off, On
; in Blitz mode, then back in Amiga mode Edit$ reads a line.  The original
; does not come back (endless loop: the session times out).
; Correct: Off puts the old input back, the second On keeps the file as
; the old handler, Edit$ reads the line of the file.
DEFTYPE .l

!TestCase{off_handler}
  f$="CORPUS:bo408.txt"
  r=WriteFile(0,f$)
  !Require{r,"written"}
  FileOutput 0 : NPrint "hello" : CloseFile 0
  DefaultOutput
  r=ReadFile(0,f$)
  !Require{r,"read_open"}
  FileInput 0
  BLITZ
  BlitzKeys On
  BlitzKeys Off
  BlitzKeys On
  AMIGA
  a$=Edit$(20)
  DefaultInput
  CloseFile 0
  KillFile f$
  !CheckStrEq{a$,"hello","line"}
!EndTestCase
