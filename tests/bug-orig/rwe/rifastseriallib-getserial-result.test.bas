; rifastseriallib-getserial-result (#649) - GetSerial address,bytes is
; declared with a #byte result like the form without arguments, but
; never sets d0: the result is the low byte of its address argument.
; The library drives the serial hardware itself (SERDAT/SERDATR); the
; remote end on the emulated serial port (serial=on,
; tests/corpus/runner/serial.h) answers request B with "HELLO".  The
; buffer comes from AllocMem (low byte a multiple of 8, never "O"); the
; original's result equals the low byte of it.
; Correct (the result of a command of the RWE libraries that works): -1
; after the block was received.
DEFTYPE .l

!TestCase{block_received}
  b=AllocMem_(16,$10001)
  !Require{b<>0,"memory"}
  r=InitSerialIO(19200,16,b,b)
  q$=Chr$(1)+"rifastseriallib-getserial-result"+Chr$(2)+Chr$(5)+"B"
  For i=1 To Len(q$) : PutSerial Asc(Mid$(q$,i,1)) : Next
  g.b=GetSerial(b,5)
  s$=""
  For i=0 To 4 : s$=s$+Chr$(Peek.b(b+i)) : Next
  PutSerial 4
  FinitSerialIO
  FreeMem_ b,16
  !CheckStrEq{s$,"HELLO","block"}
  !CheckEq{g,-1,"result"}
!EndTestCase
