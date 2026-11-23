; riseriallib-receiveblock-one (#799) - SerialReceiveBlock counts the
; bytes after the first one down to 0: with a length of 1 the count goes
; below 0 and it receives without end, behind the buffer (the session
; ends in a timeout).  The program asks the remote end (serial=on,
; tests/corpus/runner/serial.h) for "Z" and receives a block of 1 into a
; cleared buffer.
; Correct: one byte "Z", the byte behind the buffer stays 0.
DEFTYPE .l

!TestCase{receiveblock_one}
  b=AllocMem_(16,$10001)
  !Require{b<>0,"memory"}
  SerialOpen 19200
  q$=Chr$(1)+"riseriallib-receiveblock-one"+Chr$(2)+Chr$(5)+"A"
  For i=1 To Len(q$) : SerialWrite Asc(Mid$(q$,i,1)) : Next
  SerialReceiveBlock b,1
  c0=Peek.b(b) : c1=Peek.b(b+1)
  FreeMem_ b,16
  SerialWrite 4 : VWait 10
  !CheckEq{c0,Asc("Z"),"byte"}
  !CheckEq{c1,0,"byte_behind"}
!EndTestCase
