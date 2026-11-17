; ritooltypeslib-findtooltype-stale-bracket-flag (#843) - FindToolType
; tt$,1 sets a flag for the bracket search ("(tt$)", a switched-off tool
; type) that only the end of that search clears.  After a call that
; returns before it (here: FOO is found without brackets) the flag stays
; set, and the next one-argument FindToolType also searches "(tt$)".
; The icon is the one of SYS:Prefs with its tool types replaced by FOO=1
; and (BRACK).
; Correct: FindToolType("BRACK") does not find "(BRACK)": 0 both times.
; @fixed #843
; @rwe ritooltypeslib
If GetIconObject("SYS:Prefs")
  ClearToolTypes
  r=NewToolType("FOO","1")
  r=NewToolType("BRACK","",0)
  NPrint FindToolNumber(1)
  NPrint FindToolType("FOO",1)
  NPrint FindToolType("BRACK")
  NPrint FindToolType("BRACK")
Else
  NPrint "no icon"
EndIf
End
