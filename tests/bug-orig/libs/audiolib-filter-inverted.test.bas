; audiolib-filter-inverted (#985) - Filter On executes BSET #1,$bfe001 and
; Filter Off BCLR #1,$bfe001.  Bit 1 of CIA-A port A is the /LED output,
; active low (Amiga Hardware Reference Manual, CIAAPRA): 0 = power LED
; bright and audio low-pass filter on, 1 = LED dim and filter off.  The
; manual says "Filter On" turns the filter on.  The bit is read back from
; $bfe001 after each statement and the state before the test restored.
; Correct: Filter On clears the bit (0), Filter Off sets it (2).
DEFTYPE .l

!TestCase{led_bit}
  old=Peek.b($BFE001) AND 2
  Filter On
  a=Peek.b($BFE001) AND 2
  Filter Off
  b=Peek.b($BFE001) AND 2
  If old Then Filter Off Else Filter On
  !CheckEq{a,0,"filter_on_bit"}
  !CheckEq{b,2,"filter_off_bit"}
!EndTestCase
