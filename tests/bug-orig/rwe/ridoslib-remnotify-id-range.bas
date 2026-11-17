; ridoslib-remnotify-id-range (#626) - RemNotify reads the request of an
; id from a table of 64 longs without a check (the offset id*4 as a
; long), calls EndNotify and FreeMem on what it finds there and clears
; the long.  The original has no error check area.  Here with runtime
; error checking and the id 64, which reads the zero long behind the
; table in the original (harmless there).
; Correct: the error check refuses the id: "Notify number must be 0 to
; 63".
; @fixed #626
; @rwe ridoslib
; @pass timeout=30
; @pass runtime_errors=on
DEFTYPE .l
NPrint "before"
RemNotify 64
NPrint "after"
End
