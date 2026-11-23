; rilesdebuglib-finverse-detoke (#706) - FInverse sets a word; FPrint
; tests the word, FPrintDetoke only its high byte, so FInverse 1 inverts
; FPrint but not FPrintDetoke.  FPrintDetoke's inverse space marker sets
; the high byte and clears it again: after FInverse $100 and a line with
; that marker, FInverse is off (the word is 0).  The program compares the
; glyph rows in plane 0 of the used bitmap with an 'A' printed normally.
; Correct: FInverse 1 inverts FPrintDetoke too, and the marker leaves the
; FInverse setting as it was.
DEFTYPE .l

!TestCase{inverse}
  BitMap 0,640,16,1
  l=AllocMem_(16,$10001)
  !Require{l<>0,"allocated"}
  Poke$ l,"A"
  p=Peek.l(Addr BitMap(0)+8)
  FInverse 0 : FLocate 1,0 : FPrint "A"
  FInverse 1 : FLocate 0,0 : FPrintDetoke l,100,0,0
  k1=0
  For i=0 To 7
    If (Peek.b(p+i*80) AND 255)+(Peek.b(p+1+i*80) AND 255)<>255 Then k1=1
  Next
  FInverse $100 : FLocate 0,1 : FPrintDetoke l,0,1,0
  FLocate 3,1 : FPrint "A"
  k2=0
  For i=0 To 7
    If (Peek.b(p+643+i*80) AND 255)+(Peek.b(p+1+i*80) AND 255)<>255 Then k2=1
  Next
  FInverse 0
  FreeMem_ l,16
  !CheckEq{k1,0,"detoke_after_finverse_1"}
  !CheckEq{k2,0,"fprint_after_marker"}
!EndTestCase
