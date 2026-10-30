; 2dlib-allocfill-a3 (#390) - AllocFill is declared as a statement
; without a used bitmap (no !libs), but _allocfill takes the clip width
; and height of the bitmap from 56(a3)/58(a3): called from BASIC, a3 is
; the write pointer of the string work buffer, whatever the last string
; expression left there.
; Here a3 is set (inline assembler) to a zeroed block, so that the result
; does not depend on the string buffer; BitMap 0 (320x200) is used.  The
; free CHIP memory (sum of mh_Free of the CHIP MemHeaders) before and
; after AllocFill gives the size of the fill buffer.
; Correct: the buffer is sized for the used bitmap (320/8*200 = 8000
; bytes).  The original reads 0 and 0 behind a3 and allocates nothing.
; @configs clib
DEFTYPE .l
Function.l chipfree{}
  h.l=Peek.l(Peek.l(4)+322) : f.l=0
  While Peek.l(h)<>0
    If (Peek.w(h+14) AND 2)<>0 Then f=f+Peek.l(h+28)
    h=Peek.l(h)
  Wend
  Function Return f
End Function
BitMap 0,320,200,1
z=AllocMem(256,$10001)
f0=chipfree{}
GetReg d0,z : MOVE.l d0,a3
AllocFill
f1=chipfree{}
NPrint "buffer at least 8000 ",(f0-f1)>=8000
FreeFill
FreeMem z,256
End
