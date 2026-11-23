; rifastseriallib-count-zero (#1164) - InitSerialIO baud,numbytes,...
; stores numbytes - 1 for DoSerialIO's DBRA loop: numbytes 0 becomes
; $FFFF, and DoSerialIO without argument exchanges 65536 bytes, far
; beyond buffers sized for 0 (the bytes go into the memory behind the
; input buffer).  SetIOSize 0 gives the same, as it stores its count - 1
; too.  The program (slave: DoSerialIO waits for a byte, then sends one)
; gets no data from the remote end (serial=on,
; tests/corpus/runner/serial.h): the original waits for the first of the
; 65536 bytes (the session ends in a timeout).
; Correct: a count of 0 means no transfer; DoSerialIO without argument
; returns at once after InitSerialIO baud,0,... and after SetIOSize 0,
; and the input buffer stays as it was.
DEFTYPE .l

!TestCase{count_zero}
  b=AllocMem_(64,$10001)
  !Require{b<>0,"memory"}
  For i=0 To 15 : Poke.b b+32+i,Asc("x") : Next
  r=InitSerialIO(19200,0,b,b+32)
  q$=Chr$(1)+"rifastseriallib-count-zero"+Chr$(2)
  For i=1 To Len(q$) : PutSerial Asc(Mid$(q$,i,1)) : Next
  DoSerialIO
  v1=Peek.b(b)
  SetIOSize 4
  SetIOSize 0
  DoSerialIO
  v2=Peek.b(b)
  PutSerial 4
  FinitSerialIO
  FreeMem_ b,64
  !CheckEq{v1,0,"init_count_0"}
  !CheckEq{v2,0,"setiosize_0"}
!EndTestCase
