; multi-68010-vbr (#42) - the compiler and environlib, errtraplib and
; runerrlib read the VBR (MOVEC) only if AttnFlags has at least 2 of the
; CPU bits: a 68010 has only one, so its VBR is taken as 0 even when the
; vector table was moved.
; Here the case copies the vector table, moves the VBR there (exec
; Supervisor, MOVEC d7,VBR as words: the Blitz assembler is 68000 only)
; and starts a child (multi-68010-vbr.test/child.bas) with the table's
; address: the child prints whether cr_vbr (environlib, read at its start)
; is the moved table, then sets a SetErr handler and raises TRAP #0
; (errtraplib puts its TRAP #0 handler into the table at its VBR: with 0
; the moved table keeps exec's vector and the handler is never reached).
; Then the VBR goes back to 0.  Kickstart 1.3 gives no return code of the
; child.
; Correct: on the 68010 cr_vbr is the moved table and the handler runs.
DEFTYPE .l

!TestCase{moved_vbr}
  t=AllocMem_(1024,1)
  !Require{t<>0,"table"}
  CopyMem_ 0,t,1024
  v=t : Gosub vbr_set
  *r.TLRun=!RunProgram{"@TESTDIR@/child "+Str$(t),"child"}
  v=0 : Gosub vbr_set
  FreeMem_ t,1024
  !CheckStrEq{*r\printed,"cr_vbr is the moved table -1"+Chr$(10)+"SetErr handler reached"+Chr$(10),"printed"}
  !CheckEq{*r\rcknown,0,"no_rc"}
  Goto vbr_done
vbr_set:
  GetReg d0,v
  MOVEM.l d2-d7/a2-a6,-(a7)
  MOVE.l d0,d7 : LEA vbr_sv(pc),a5 : MOVE.l 4,a6 : JSR -30(a6)
  MOVEM.l (a7)+,d2-d7/a2-a6
  Return
vbr_sv:
  Dc.w $4E7B,$7801
  RTE
vbr_done:
!EndTestCase
