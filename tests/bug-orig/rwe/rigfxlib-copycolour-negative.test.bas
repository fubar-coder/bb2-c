; rigfxlib-copycolour-negative (#678) - CopyColour checks the colour
; numbers against the palettes' numbers of colours with signed compares
; (CMP.w : BGE) only from above: a negative colour number passes and is
; indexed with MULU.w #12 (-1 = 786420 bytes behind colour 0); the
; result is -1 (success).  Made safe: the colour data of palette 0 are
; temporarily a block of the program's own (800000 bytes), so the
; source colour -1 is read inside it.
; Correct: CopyColour refuses the colour -1 and returns 0.
DEFTYPE .l

!TestCase{negative_colour}
  InitPalette 0,4 : InitPalette 1,4
  b=AllocMem_(800000,$10001)
  !Require{b<>0,"memory"}
  Poke.w b,4
  q=Addr Palette(0) : o=Peek.l(q) : Poke.l q,b
  r=CopyColour(0,1,-1,0)
  Poke.l q,o
  FreeMem_ b,800000
  !CheckEq{r,0,"result"}
!EndTestCase
