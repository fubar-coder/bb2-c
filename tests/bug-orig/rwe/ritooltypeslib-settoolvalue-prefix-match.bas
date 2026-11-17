; ritooltypeslib-settoolvalue-prefix-match (#853) - SetToolValue looks
; for the entry to replace by comparing the name up to its end only: an
; entry that just begins with the name matches.  The icon is the one of
; SYS:Prefs with its tool types replaced by FLAG and FL=1;
; SetToolValue "FL","x" replaces FLAG, the first entry that begins with
; "FL".
; Correct: FL=1 becomes FL=x, FLAG stays.
; @fixed #853
; @rwe ritooltypeslib
If GetIconObject("SYS:Prefs")
  ClearToolTypes
  r=NewToolType("FLAG","")
  r=NewToolType("FL","1")
  NPrint SetToolValue("FL","x")
  NPrint FindToolNumber(0)
  NPrint FindToolNumber(1)
Else
  NPrint "no icon"
EndIf
End
