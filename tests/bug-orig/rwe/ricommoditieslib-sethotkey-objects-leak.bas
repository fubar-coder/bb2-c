; ricommoditieslib-sethotkey-objects-leak (#563) - SetHotKey with a number
; that already has a hotkey removes the old filter and its sender from
; the broker (RemoveCxObj) but never deletes them: two commodity objects
; lost per call, also at the end of the program.  The program replaces
; hotkey 0 1000 times and compares the free memory before and after (one
; replacement before the first measurement).
; Correct: the memory comes back - less than 20000 bytes missing (the
; leak is more than 100 bytes per call).
; @fixed #563
; @rwe ricommoditieslib
DEFTYPE .l
If MakeCommodity("BugOrigLeak","BugOrig","replace")
  r=SetHotKey(0,"lalt f1")
  r=SetHotKey(0,"lalt f1")
  m0=AvailMem_(0)
  For i=1 To 1000
    r=SetHotKey(0,"lalt f1")
  Next
  m1=AvailMem_(0)
  NPrint r
  NPrint "leaked ",m0-m1>20000
Else
  NPrint "no commodity"
EndIf
End
