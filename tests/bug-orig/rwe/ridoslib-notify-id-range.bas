; ridoslib-notify-id-range (#626) - AddNotify keeps the request and the
; length of the name copy in two tables of 64 longs and uses the id as
; the index without a check (the offset id*4 as a word): an id outside
; 0..63 writes the library's data around the tables (larger ids also
; the variables behind them).  The original has no error check area.
; Here with runtime error checking and the id -1, which writes the zero
; longs in front of the two tables (harmless in the original and in the
; port); the file lies on a missing volume (the process's requesters
; off: pr_WindowPtr -1, put back after), so StartNotify fails and no
; request stays active.
; Correct: the error check refuses the id: "Notify number must be 0 to
; 63".
; @fixed #626
; @rwe ridoslib
; @pass timeout=30
; @pass runtime_errors=on
DEFTYPE .l
p=Peek.l(Peek.l(4)+276)
w=Peek.l(p+184)
NPrint "before"
Poke.l p+184,-1
r=AddNotify(-1,"RWENOSUCHDEVICE:x")
Poke.l p+184,w
NPrint "after ",r
End
