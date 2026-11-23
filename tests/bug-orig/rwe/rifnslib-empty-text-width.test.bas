; rifnslib-empty-text-width (#659) - FNSPrint counts the width of a line
; and stores width + 1; the binary has no guard for a width of 0, so an
; empty text (or a line that starts with Chr$(10)) counts 1 pixel:
; underlined, it sets one pixel at the print position; centred, x moves
; by -32767.  Here an empty text and a text of two Chr$(10) (three empty
; lines), both underlined, in the built-in font.
; Correct: nothing is drawn for an empty line; the bitmap stays empty
; ("longs with pixels: 0").
DEFTYPE .l

!TestCase{empty_text}
  BitMap 0,320,64,1
  FNSOutput 0
  FNSClip 0,0,319,63
  FNSPrint 0,100,30,"",4,1
  FNSPrint 0,200,10,Chr$(10)+Chr$(10),4,1
  p=Peek.l(Addr BitMap(0)+8)
  s=0
  For i=0 To 40*64-4 Step 4
    If Peek.l(p+i) Then s=s+1
  Next
  !CheckEq{s,0,"longs_with_pixels"}
!EndTestCase
