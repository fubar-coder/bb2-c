; rifastseriallib-finit-interrupts (#651, #648) - InitSerialIO switches
; the serial interrupts TBE and RBF off and is meant to remember which
; were on, so that FinitSerialIO can switch them on again; it stores the
; low word of the address of INTENAR ($F01C) instead of its value.  With
; bits 11 and 0 of $F01C clear FinitSerialIO writes nothing: RBF stays
; off.  Its write would go to the absolute address $9A (the low word of
; the TRAP #6 vector), not to INTENA.  The program opens serial.device
; first (it switches RBF on), takes the RBF bit of INTENAR before
; InitSerialIO, after it and after FinitSerialIO, and whether the word at
; $9A changed.
; Correct: RBF is on again after FinitSerialIO, the word at $9A stays.
DEFTYPE .l

Even
.inbuf
Dcb.b 64,0
.outbuf
Dcb.b 64,0

!TestCase{rbf_restored}
  u=OpenSerial("serial.device",0,9600,0)
  !Require{u<>0,"serial_open"}
  w=Peek.w($9A)
  a=Peek.w($DFF01C) AND $0800
  r=InitSerialIO(19200,16,?inbuf,?outbuf)
  b=Peek.w($DFF01C) AND $0800
  FinitSerialIO
  c=Peek.w($DFF01C) AND $0800
  v=Peek.w($9A)
  CloseSerial 0
  !CheckEq{a,2048,"rbf_before"}
  !CheckEq{b,0,"rbf_after_init"}
  !CheckEq{c,2048,"rbf_after_finit"}
  !CheckEq{v,w,"vector_kept"}
!EndTestCase
