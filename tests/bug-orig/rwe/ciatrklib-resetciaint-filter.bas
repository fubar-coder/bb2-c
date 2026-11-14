; ciatrklib-resetciaint-filter (#449) - ResetCIAInt ends with
; BCHG #1,$bfe001: every removal of the CIA interrupt toggles bit 1 of
; CIA-A port A, the audio low-pass filter and the power LED (at
; ChangeTrackerVBlank, at every StartTracker in CIA mode and at the end of
; the program).
; Here: the library's init has installed the CIA interrupt;
; ChangeTrackerVBlank removes it, the bit is read before and after.
; Correct: the filter bit stays as it was.
; @fixed #449
; @rwe ciatrklib
; @pass timeout=30
DEFTYPE .l
b0=Peek.b($BFE001) AND 2
ChangeTrackerVBlank
b1=Peek.b($BFE001) AND 2
If b0=b1 Then NPrint "filter bit kept" Else NPrint "filter bit toggled"
End
