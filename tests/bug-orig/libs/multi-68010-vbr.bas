; multi-68010-vbr (#42) - the compiler and environlib, errtraplib and
; runerrlib read the VBR (MOVEC) only if AttnFlags has at least 2 of the
; CPU bits: a 68010 has only one, so its VBR is taken as 0 even when the
; vector table was moved.
; Here the program copies the vector table, moves the VBR there (exec
; Supervisor, MOVEC d7,VBR as words: the Blitz assembler is 68000 only)
; and runs itself a second time (dos Execute, output to the same file):
; the child prints whether cr_vbr (environlib, read at its start) is the
; moved table.  Then the VBR goes back to 0.
; Correct: on the 68010 cr_vbr is the moved table.
; @machine a500-68010
DEFTYPE .l
If NumPars=0
  t=AllocMem_(1024,1)
  CopyMem_ 0,t,1024
  v=t : Gosub setvbr
  c$="CORPUS:out/multi-68010-vbr "+Str$(t)
  Execute_ &c$,0,Output_
  v=0 : Gosub setvbr
  FreeMem_ t,1024
  End
EndIf
a$=Par$(1) : p=0
For i=1 To Len(a$) : p=p*10+Asc(Mid$(a$,i,1))-48 : Next
NPrint "cr_vbr is the moved table ",cr_vbr=p
End

setvbr:
GetReg d0,v
MOVEM.l d2-d7/a2-a6,-(a7)
MOVE.l d0,d7 : LEA sv(pc),a5 : MOVE.l 4,a6 : JSR -30(a6)
MOVEM.l (a7)+,d2-d7/a2-a6
Return

sv:
  Dc.w $4E7B,$7801
  RTE
