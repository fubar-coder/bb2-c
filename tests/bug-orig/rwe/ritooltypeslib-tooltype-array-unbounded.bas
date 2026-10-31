; ritooltypeslib-tooltype-array-unbounded (#855) - the tool types of the
; open icon live in the library's array of 100 longs; NewToolType looks
; for the first free entry and clears the entry behind it without a
; count.  With 99 entries in use the 100th NewToolType takes the last
; entry and clears the long behind the array (and leaves the array
; without its terminating 0); it returns -1 (success).  The icon is the
; one of SYS:Prefs, its tool types cleared first.
; Correct: the array keeps a terminating 0 - the 100th NewToolType fails
; (returns 0).
; @rwe ritooltypeslib
; @pass timeout=30
If GetIconObject("SYS:Prefs")
  ClearToolTypes
  n=0
  For i=1 To 99
    If NewToolType("T"+Str$(i),"1") Then n=n+1
  Next
  NPrint n," added"
  NPrint NewToolType("LAST","1")
Else
  NPrint "no icon"
EndIf
End
