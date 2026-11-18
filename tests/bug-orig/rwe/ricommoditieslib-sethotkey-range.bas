; ricommoditieslib-sethotkey-range (#564) - SetHotKey uses the number as
; the index into its tables of 16 filters and senders without a range
; check.  With -1 the "filter" slot is the variable in front of the
; table, the broker itself: it is removed with RemoveCxObj (and the
; variable overwritten with the new filter).  The program shows that
; through a second broker of the same name: commodities refuses it
; (NBU_UNIQUE) while the first one is still there.  The original has no
; error check area; here with runtime error checking.
; Correct: the error check refuses the number: "Hotkey number must be 0
; to 15".
; @fixed #564
; @rwe ricommoditieslib
; @pass runtime_errors=on
If MakeCommodity("BugOrigRange","BugOrig","range")
  NPrint "before"
  NPrint SetHotKey(-1,"lalt f1")
  NPrint MakeCommodity("BugOrigRange","BugOrig","range")
Else
  NPrint "no commodity"
EndIf
End
