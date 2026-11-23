; ciatrklib-resetciaint-filter (#449) - ResetCIAInt ends with
; BCHG #1,$bfe001: every removal of the CIA interrupt toggles bit 1 of
; CIA-A port A, the audio low-pass filter and the power LED (at
; ChangeTrackerVBlank, at every StartTracker in CIA mode and at the end of
; the program).
; Here: the library's init has installed the CIA interrupt;
; ChangeTrackerVBlank removes it, the bit is read before and after.
; Correct: the filter bit stays as it was.
DEFTYPE .l

!TestCase{filter_bit}
  b0=Peek.b($BFE001) AND 2
  ChangeTrackerVBlank
  b1=Peek.b($BFE001) AND 2
  !CheckEq{b1,b0,"filter_bit_kept"}
!EndTestCase
