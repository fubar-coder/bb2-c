; compiler-reloc32-one-block (#943) - the executable writer puts all
; relocations of the code hunk into one HUNK_RELOC32 block.  LoadSeg of
; AmigaOS V36+ does not load a block of more than 65535 offsets: on
; AmigaOS 3.2 the executable fails with "file is not executable" (rc=10).
; Kickstart 1.3 loads it with every relocation applied (checked with
; 8 MB fast memory: the compile needs about 36 bytes per relocation, it
; runs out of memory with 1 MB chip + 1 MB slow; about 10 minutes), so the
; test runs only on the A1200.  A program with 60000 relocations loads
; correctly on both.
; The macros m1..m4 give 16, 256, 4096 and 65536 Dc.l of the label t, the
; table below t: 69632 absolute relocations (with those of the program
; itself about 69700 in the block).  check compares every entry with the
; address of t taken PC-relative (no relocation) and counts the correct
; ones; a loader that skips or misplaces relocations (also those after
; the 65536th) leaves wrong entries and prints the first.
; Correct: the program loads and prints 69632 of 69632 entries correct.
DEFTYPE .l
Macro m1
Dc.l t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t
End Macro
Macro m2
!m1:!m1:!m1:!m1:!m1:!m1:!m1:!m1:!m1:!m1:!m1:!m1:!m1:!m1:!m1:!m1
End Macro
Macro m3
!m2:!m2:!m2:!m2:!m2:!m2:!m2:!m2:!m2:!m2:!m2:!m2:!m2:!m2:!m2:!m2
End Macro
Macro m4
!m3:!m3:!m3:!m3:!m3:!m3:!m3:!m3:!m3:!m3:!m3:!m3:!m3:!m3:!m3:!m3
End Macro
n=69632 : Gosub check
NPrint ok," of ",n," entries correct"
If bad>=0
  NPrint "first wrong entry ",bad,", value-address ",dif
EndIf
End

check:
GetReg d0,n
MOVEM.l d2-d7/a2-a6,-(a7)
MOVE.l d0,d2
LEA t(pc),a0
MOVE.l a0,d1
MOVEQ #0,d0
MOVEQ #-1,d3
MOVEQ #0,d4
MOVEQ #0,d5
rloop:
CMP.l (a0)+,d1
BEQ rok
TST.l d3
BPL rnext
MOVE.l d4,d3
MOVE.l -4(a0),d5
SUB.l d1,d5
BRA rnext
rok:
ADDQ.l #1,d0
rnext:
ADDQ.l #1,d4
CMP.l d2,d4
BNE rloop
PutReg d0,ok
PutReg d3,bad
PutReg d5,dif
MOVEM.l (a7)+,d2-d7/a2-a6
Return

t:
!m4
!m3
