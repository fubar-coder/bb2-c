; riseriallib-vbr (#804) - SerialOpen writes its interrupt handlers into
; the autovectors at $64/$74 and SerialClose puts the old ones back there,
; whatever the VBR of a 68010 or better is: with the vector table moved
; the handlers are never called.  The program copies the vector table,
; moves the VBR there (exec Supervisor, MOVEC d7,VBR as words: the Blitz
; assembler is 68000 only), and compares the longs at $64/$74 of both
; tables before SerialOpen, after it and after SerialClose; then the VBR
; goes back to 0.  The machine is the A500 with a 68010.
; Correct: the handlers go into the moved table (the one at 0 stays), and
; SerialClose puts the old vectors back there.
DEFTYPE .l

!TestCase{vbr}
  t=AllocMem_(1024,1)
  !Require{t<>0,"memory"}
  CopyMem_ 0,t,1024
  v=t : Gosub setvbr
  o64=Peek.l(t+$64) : o74=Peek.l(t+$74) : a64=Peek.l($64) : a74=Peek.l($74)
  SerialOpen 19200
  m64=Peek.l(t+$64) : m74=Peek.l(t+$74) : z64=Peek.l($64) : z74=Peek.l($74)
  SerialClose
  r64=Peek.l(t+$64) : r74=Peek.l(t+$74) : y64=Peek.l($64) : y74=Peek.l($74)
  v=0 : Gosub setvbr
  FreeMem_ t,1024
  !Check{m64<>o64 AND m74<>o74,"moved_table_handlers"}
  !Check{z64=a64 AND z74=a74,"table_at_0_unchanged"}
  !Check{(r64=o64) AND (r74=o74) AND (y64=a64) AND (y74=a74),"close_restores"}
  Goto vbrend

.setvbr
  GetReg d0,v
  MOVEM.l d2-d7/a2-a6,-(a7)
  MOVE.l d0,d7 : LEA sv(pc),a5 : MOVE.l 4,a6 : JSR -30(a6)
  MOVEM.l (a7)+,d2-d7/a2-a6
  Return

.sv
  Dc.w $4E7B,$7801
  RTE

.vbrend
!EndTestCase
