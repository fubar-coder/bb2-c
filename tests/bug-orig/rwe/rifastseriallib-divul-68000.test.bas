; rifastseriallib-divul-68000 (#647) - InitSerialIO computes SERPER with
; DIVU.L, a 68020 instruction (written as data, $4C40 $1001): on a 68000
; it is an illegal instruction (the session ends in a timeout).  The
; machine is the A600 (68000).
; Correct: InitSerialIO works on a 68000.
DEFTYPE .l

Even
.inbuf
Dcb.b 64,0
.outbuf
Dcb.b 64,0

!TestCase{init_on_68000}
  r=InitSerialIO(9600,16,?inbuf,?outbuf)
  !Check{1,"init_returned"}
  FinitSerialIO
!EndTestCase
