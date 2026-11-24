; riapplib-addappicon-a3-a4 (#516) - AddAppIcon passes the DiskObject
; to workbench.library's AddAppIconA in a3 and the tag list (0) in a4 and
; restores neither.  a4 is the base of the locals of a Statement or
; Function: a local read after AddAppIcon is read at a small offset from
; 0, and the procedure's UNLK a4 at its end loads the stack pointer from
; 0.  a3 is the write pointer of the string buffer: as a function in a
; string expression the rest of the expression is written into the
; DiskObject.  The icon is the volume's (SYS:Disk); whether Workbench
; takes the App icon does not matter, the registers are set before the
; call.  The program checks the length and the characters of
; "ab"+Str$(AddAppIcon(...))+"cd", then a local after AddAppIcon inside a
; Statement.
; To give the wrong a4 a defined end the Statement saves a4 in sv (a long
; in its own code, behind a Goto) before the call and puts it back after
; the check of the local, so its UNLK a4 returns although the original
; leaves a4 changed.  This relies on a4 being the base of the locals in a
; Statement (details of the code generator, not of the bug); the failed
; checks of the local and of the string show the bug.
; Correct: "ab", the result, "cd"; the local is 7; the Statement returns.
DEFTYPE .l

Statement appicon{}
  a.l=7
  LEA sv(pc),a0 : MOVE.l a4,(a0)
  r.l=AddAppIcon(2,"T","SYS:Disk")
  !CheckEq{a,7,"local"}
  LEA sv(pc),a0 : MOVE.l (a0),a4
  Goto fin
sv:
  Dc.l 0
fin:
End Statement

!TestCase{icon_registers}
  a$="ab"+Str$(AddAppIcon(1,"T","SYS:Disk"))+"cd"
  !CheckEq{Len(a$),5,"length"}
  !CheckStrEq{a$,"ab0cd","string"}
  appicon{}
  !Check{-1,"statement_returned"}
!EndTestCase
