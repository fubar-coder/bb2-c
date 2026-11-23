; riseriallib-readlong (#794) - SerialReadLong never returns: its loop
; counter is taken back from the stack every time, it reads bytes without
; end (the session ends in a timeout).  The program asks the remote end
; (serial=on, tests/corpus/runner/serial.h) for "0123456789ABCDEF" and
; reads one long.
; Correct (decided: the low byte first, the mirror of SerialWriteLong):
; the first four bytes "0123" as $33323130.
DEFTYPE .l

!TestCase{readlong}
  SerialOpen 19200
  q$=Chr$(1)+"riseriallib-readlong"+Chr$(2)+Chr$(5)+"C"
  For i=1 To Len(q$) : SerialWrite Asc(Mid$(q$,i,1)) : Next
  v=SerialReadLong
  SerialWrite 4 : VWait 10
  !CheckEq{v,$33323130,"long"}
!EndTestCase
