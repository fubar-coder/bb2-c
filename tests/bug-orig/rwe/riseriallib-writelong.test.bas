; riseriallib-writelong (#805) - SerialWriteLong sends the low byte of its
; argument and then three times $01: it rotates the register SerialWrite
; changed (the word $01xx) instead of the argument.  The program sends
; $44434241 to the remote end (serial=on, tests/corpus/runner/serial.h),
; which echoes the data bytes (request E), and reads the echo (each empty
; read followed by a VWait, at most 100).  (A $01 starts a new session at
; the remote end: the original's request is not answered.)
; Correct (decided: the low byte first): the bytes "ABCD".
DEFTYPE .l

!TestCase{writelong}
  SerialOpen 19200
  q$=Chr$(1)+"riseriallib-writelong"+Chr$(2)
  For i=1 To Len(q$) : SerialWrite Asc(Mid$(q$,i,1)) : Next
  SerialWriteLong $44434241
  SerialWrite 5 : SerialWrite Asc("E")
  r$="" : n=0
  While Len(r$)<4 AND n<100
    c=SerialRead
    If c=-1 Then VWait : n+1 Else r$+Chr$(c AND $FF)
  Wend
  SerialWrite 4 : VWait 10
  !CheckStrEq{r$,"ABCD","echo"}
!EndTestCase
