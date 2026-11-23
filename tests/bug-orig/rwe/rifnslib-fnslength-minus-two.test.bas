; rifnslib-fnslength-minus-two (#662) - FNSLength adds up the widths of
; the characters as FNSPrint does and then subtracts 2 (0 at least); the
; manual: it "returns the x size, in pixels, of the string if it were to
; be printed".  The program adds up the widths of "Hello" from the width
; table of the built-in font (FNSSlot: the font's address; the width of
; code c at +c).
; Correct: FNSLength(0,"Hello") is that sum (the two numbers are equal).
DEFTYPE .l

!TestCase{minus_two}
  f=Peek.l(FNSSlot)
  t$="Hello"
  s=0
  For i=1 To Len(t$)
    s=s+Peek.b(f+Asc(Mid$(t$,i,1)))
  Next
  l=FNSLength(0,t$,0)
  !CheckEq{l,s,"sum_of_widths"}
!EndTestCase
