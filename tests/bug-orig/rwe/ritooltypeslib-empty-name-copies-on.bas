; ritooltypeslib-empty-name-copies-on (#841) - SetToolValue/NewToolType
; build the new entry with a loop that copies a byte of the name first
; and tests the next byte afterwards: for an empty name it copies the
; name's 0 and the bytes behind it.  The entry begins with 0: the new
; tool type reads back as "".  The icon is the one of SYS:Prefs, its tool
; types cleared first.
; Correct: the entry is the name (nothing) and "=" and the value: "=abc".
; @rwe ritooltypeslib
If GetIconObject("SYS:Prefs")
  ClearToolTypes
  NPrint NewToolType("","abc")
  NPrint "entry <",FindToolNumber(0),">"
Else
  NPrint "no icon"
EndIf
End
