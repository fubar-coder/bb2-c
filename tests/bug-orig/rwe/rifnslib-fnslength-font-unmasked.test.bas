; rifnslib-fnslength-font-unmasked (#660) - every rifnslib command takes
; the font number & 15, FNSLength does not (ASL.w #2 of the number): 16
; reads the long behind the 16 slots - the first bytes of the built-in
; font, "FNS." - as the address of a font, and gives -1 or the width in
; whatever lies there.
; Correct: FNSLength(16,...) is FNSLength(0,...), as for the other
; commands (the two numbers are the same).
DEFTYPE .l

!TestCase{font_16}
  a=FNSLength(0,"xyz")
  b=FNSLength(16,"xyz")
  !CheckEq{b,a,"same"}
!EndTestCase
