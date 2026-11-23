; rifastseriallib-setiosize-count (#1158) - InitSerialIO stores its byte
; count - 1 for DoSerialIO's DBRA loop, SetIOSize stores its argument as
; it is: after SetIOSize 16 DoSerialIO transfers 17 bytes, one more than
; its buffers hold.  The program (slave: DoSerialIO reads a byte, then
; sends one) asks the remote end (serial=on,
; tests/corpus/runner/serial.h) for the 16 bytes "0123456789ABCDEF",
; sets SetIOSize 16 and runs DoSerialIO: the original waits for a 17th
; byte that never comes (the session ends in a timeout).
; Correct: SetIOSize 16 gives cycles of 16 bytes; the byte behind the
; input buffer stays 0.
DEFTYPE .l

!TestCase{size_16}
  b=AllocMem_(64,$10001)
  !Require{b<>0,"memory"}
  For i=0 To 15 : Poke.b b+32+i,Asc("x") : Next
  r=InitSerialIO(19200,4,b,b+32)
  q$=Chr$(1)+"rifastseriallib-setiosize-count"+Chr$(2)+Chr$(5)+"C"
  For i=1 To Len(q$) : PutSerial Asc(Mid$(q$,i,1)) : Next
  SetIOSize 16
  DoSerialIO
  s$=""
  For i=0 To 15 : s$=s$+Chr$(Peek.b(b+i)) : Next
  v=Peek.b(b+16)
  PutSerial 4
  FinitSerialIO
  FreeMem_ b,64
  !CheckStrEq{s$,"0123456789ABCDEF","received"}
  !CheckEq{v,0,"byte_behind"}
!EndTestCase
