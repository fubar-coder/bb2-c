; riseriallib-serialgot-count (#802) - SerialGot, whose help text says it
; "returns num chars in serial buffer", returns -1 when a byte is waiting
; and 0 otherwise.  The program asks the remote end (serial=on,
; tests/corpus/runner/serial.h) for "0123456789ABCDEF", waits a second
; and prints SerialGot.
; Correct: the number of bytes waiting, 16.
DEFTYPE .l

!TestCase{serialgot_count}
  SerialOpen 19200
  q$=Chr$(1)+"riseriallib-serialgot-count"+Chr$(2)+Chr$(5)+"C"
  For i=1 To Len(q$) : SerialWrite Asc(Mid$(q$,i,1)) : Next
  n=0
  While n<50 : VWait : n+1 : Wend
  g=SerialGot
  SerialFlush
  SerialWrite 4 : VWait 10
  !CheckEq{g,16,"got"}
!EndTestCase
