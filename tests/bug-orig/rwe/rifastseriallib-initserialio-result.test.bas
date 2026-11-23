; rifastseriallib-initserialio-result (#650) - InitSerialIO is declared
; with a #long result but returns its baud argument, also the second
; time, when the library is already initialised and it does nothing.
; Correct (the result of a command of the RWE libraries that works: -1
; when it set the port up, 0 when it did nothing): -1, then 0.
DEFTYPE .l

Even
.inbuf
Dcb.b 64,0
.outbuf
Dcb.b 64,0

!TestCase{two_inits}
  r1=InitSerialIO(19200,16,?inbuf,?outbuf)
  r2=InitSerialIO(9600,16,?inbuf,?outbuf)
  FinitSerialIO
  !CheckEq{r1,-1,"first_result"}
  !CheckEq{r2,0,"second_result"}
!EndTestCase
