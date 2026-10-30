; ricommoditieslib-sethotkey-range (#564) - SetHotKey uses the number as
; the index into its tables of 16 filters and senders without a range
; check.  With -1 the "filter" slot is the variable in front of the
; table, the broker itself: it is removed with RemoveCxObj (and the
; variable overwritten with the new filter).  The program shows that
; through a second broker of the same name: commodities refuses it
; (NBU_UNIQUE) while the first one is still there.
; Correct: SetHotKey -1 is refused (0), the broker stays, the second
; MakeCommodity of the name fails (0).
; @rwe ricommoditieslib
If MakeCommodity("BugOrigRange","BugOrig","range")
  NPrint SetHotKey(-1,"lalt f1")
  NPrint MakeCommodity("BugOrigRange","BugOrig","range")
Else
  NPrint "no commodity"
EndIf
End
