; riarexxlib-makemsg-allocvec-flags (#533) - _makemsg tests the flags
; that AllocVec leaves, not its result, and on failure leaves thismsg
; (the message made or received last) unchanged; the callers go on with
; it.  The program runs through inject with the second message's AllocVec
; failing (runwrap=, inject allocfail: 128 bytes + 4; the first one
; succeeds).  It sends "one" to its own port and receives it (the
; current message), then sends "two": the failure reuses "one", writes
; the new argument into it and puts it to the port again.
; Correct: the failed RXRunCommand sends nothing - no message arrives.
; @rwe riarexxlib
; @pass runwrap=@INJECT@|allocfail=132-132/1+1
DEFTYPE .l
p=RXCreatePort("BUGORIG")
RXTokenise On
RXRunCommand "BUGORIG","one"
m1=RXReadPort
NPrint RXGetArg(1)
RXRunCommand "BUGORIG","two"
m2=RXReadPort
NPrint "again ",m2=m1 AND m2<>0
If m2 Then NPrint RXGetArg(1)
End
