; ritooltypeslib-tooltype-array-unbounded (#855) - the tool types of the
; open icon live in the library's array of 100 longs; NewToolType looks
; for the first free entry and clears the entry behind it without a
; count.  With 99 entries in use the 100th NewToolType takes the last
; entry and clears the long behind the array (and leaves the array
; without its terminating 0); it returns -1 (success).  The icon is the
; one of SYS:Prefs, its tool types cleared first.  What lies behind the
; array differs with the build (in the C library: code of the next
; routine, which then crashes): the program saves that long and puts it
; back right after the 100th NewToolType, and clears the last entry
; again, so only the return value is observed.
; Correct: the array keeps a terminating 0 - the 100th NewToolType fails
; (returns 0).
; @rwe ritooltypeslib
; @pass timeout=30
DEFTYPE .l
d=GetIconObject("SYS:Prefs")
If d
  a=Peek.l(d+54)
  o=Peek.l(a+400)
  ClearToolTypes
  n=0
  For i=1 To 99
    If NewToolType("T"+Str$(i),"1") Then n=n+1
  Next
  NPrint n," added"
  r=NewToolType("LAST","1")
  Poke.l a+400,o
  Poke.l a+396,0
  NPrint r
Else
  NPrint "no icon"
EndIf
End
