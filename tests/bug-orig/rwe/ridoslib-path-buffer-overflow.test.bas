; ridoslib-path-buffer-overflow (#627) - GetFileName builds its result in
; tbuf (256 bytes, in the code hunk) without a limit: a longer file part
; runs over the code behind it, first NotifyPort (MOVE.l msgport,d0 :
; RTS).  The name is 300 characters "NuNu...": behind tbuf they are the
; words $4E75 (RTS), so NotifyPort returns at once with whatever d0
; holds.  (The C build overwrites other data of the library instead.)
; Correct: tbuf is not overrun, NotifyPort returns the same port as
; before.
DEFTYPE .l

!TestCase{long_name}
  p=NotifyPort
  a$=String$("Nu",150)
  b$=GetFileName(a$)
  !CheckEq{Len(b$),300,"name_length"}
  !CheckEq{NotifyPort,p,"port_as_before"}
!EndTestCase
