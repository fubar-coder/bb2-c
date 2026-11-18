; ricommoditieslib-makecommodity-twice (#560) - MakeCommodity makes a new
; broker on every call without deleting the one it has: a second call
; with another name succeeds, the first broker stays active with the
; same message port, and finit deletes only the second, so the first
; is left registered with commodities and a deleted port.
; Correct: a second MakeCommodity returns 0 while the library has a
; broker; the first one stays in use (user decision W4 Q4).
; @fixed #560
; @rwe ricommoditieslib
If MakeCommodity("BugOrigTwiceA","BugOrig","first")
  NPrint MakeCommodity("BugOrigTwiceB","BugOrig","second")
Else
  NPrint "no commodity"
EndIf
End
