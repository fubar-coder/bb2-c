; riarexxlib-makemsg-allocvec-flags (#533) - _makemsg tests the flags
; that AllocVec leaves, not its result, and on failure leaves thismsg
; (the message made or received last) unchanged; the callers go on with
; it.  The program runs through inject with the second message's AllocVec
; failing (runwrap=, inject allocfail: 128 bytes + 4; the first one
; succeeds).  It sends "one" to its own port and receives it (the
; current message), then sends "two": the failure reuses "one", writes
; the new argument into it and puts it to the port again.
; Correct: the failed RXRunCommand sends nothing - no message arrives.
; The message sent to the own port is never replied: finit waits 5 s for
; its reply (#1095), silent; the hang check gets 20 s instead of 8.
DEFTYPE .l

!TestCase{second_alloc_fails}
  p=RXCreatePort("BUGORIG")
  RXTokenise On
  RXRunCommand "BUGORIG","one"
  m1=RXReadPort
  a$=RXGetArg(1)
  !CheckStrEq{a$,"one","first_arg"}
  RXRunCommand "BUGORIG","two"
  m2=RXReadPort
  again=(m2=m1 AND m2<>0)
  !CheckEq{again,0,"again"}
!EndTestCase
