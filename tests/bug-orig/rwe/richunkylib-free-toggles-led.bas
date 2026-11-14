; richunkylib-free-toggles-led (#886) - freeing the data of a chunky
; bitmap ends with BCHG #1,$bfe001: every free toggles bit 1 of CIA-A
; port A, the power LED and the audio low-pass filter.
; Here: ChunkyBitmap on an object that already holds data (frees the old
; data first), then the bit read before and after.
; Correct: the filter bit stays as it was.
; @fixed #886
; @rwe richunkylib
; @pass timeout=30
DEFTYPE .l
ChunkyBitmap 0,32,16
b0=Peek.b($BFE001) AND 2
ChunkyBitmap 0,32,16
b1=Peek.b($BFE001) AND 2
If b0=b1 Then NPrint "filter bit kept" Else NPrint "filter bit toggled"
End
