; ricommoditieslib-sethotkey-result (#565) - SetHotKey never calls
; CxObjError: a hotkey description that commodities' ParseIX does not
; understand still gives a filter (with COERR_BADFILTER), which is
; attached and reported as success (-1), though it never triggers.  (It
; also tests the d0 that the void AttachCxObj leaves; with Workbench 3.1's
; commodities.library that is not 0.)
; Correct: SetHotKey with an invalid description returns 0; a valid one
; -1.
; @fixed #565
; @rwe ricommoditieslib
If MakeCommodity("BugOrigResult","BugOrig","bad hotkey")
  NPrint SetHotKey(1,"rawkey nonsense_qualifier q")
  NPrint SetHotKey(2,"lalt f1")
Else
  NPrint "no commodity"
EndIf
End
