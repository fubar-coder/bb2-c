; riseriallib-readstring-limit (#1161) - SerialReadString stores received
; bytes at a3 until a 0 arrives, without a limit: a sender that sends no
; 0 writes over the string work area and beyond (the session ends in a
; timeout).  The string work area is 1000 bytes (tedstub strbuf=); the
; program asks the remote end (serial=on, tests/corpus/runner/serial.h)
; for 1050 bytes without a 0 and reads them with SerialReadString.
; Correct: SerialReadString stops at the end of the string work area (and
; stores the 0 there): a string of 999 characters.
DEFTYPE .l

!TestCase{readstring_limit}
  SerialOpen 19200
  q$=Chr$(1)+"riseriallib-readstring-limit"+Chr$(2)+Chr$(5)+"N1050"
  For i=1 To Len(q$) : SerialWrite Asc(Mid$(q$,i,1)) : Next
  a$=SerialReadString
  n=Len(a$)
  SerialFlush
  SerialWrite 4 : VWait 10
  !CheckEq{n,999,"length"}
!EndTestCase
