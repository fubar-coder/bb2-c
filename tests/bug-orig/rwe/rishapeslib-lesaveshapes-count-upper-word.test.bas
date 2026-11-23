; rishapeslib-lesaveshapes-count-upper-word (#815) - LESaveShapes computes
; the shape count of the SHHD chunk as last - first with a word
; subtraction into the long it stores: the upper word of d1 stays.
; Compiled code leaves one there when the last shape is a long
; expression: a=$50009 passes shape 9 with 5 in the upper word.  Shapes
; 0..9 are saved so and the count is read from the file (offset 20).
; Correct: the count 9 (the shapes - 1); the original writes $50009.
; rishapeslib needs RIPackLib (PackPBM, UnPackPBM), which needs
; RIAmosFuncLib (banks).
DEFTYPE .l

!TestCase{count_upper_word}
  For i=0 To 9 : LEInitShape i,16,8,1 : Next
  a=$50009
  LESaveShapes 0,a,"CORPUS:w.le"
  m=AllocMem_(1024,$10001)
  !Require{m<>0,"memory"}
  n$="CORPUS:w.le"
  f=Open_(&n$,1005)
  !Require{f<>0,"file"}
  r=Read_(f,m,1024)
  Close_ f
  c=Peek.l(m+20)
  FreeMem_ m,1024
  !CheckEq{c,9,"count"}
!EndTestCase
