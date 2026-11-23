; seriallib-openserial-reopen-fail (#260) - a second OpenSerial of a unit
; that is open (exclusive access, io_SerFlags 0) sets up the read half
; again (port, signal, request pointer) before OpenDevice, and when
; OpenDevice fails its error path clears the read half's request
; pointer: the unit counts as not open, CloseSerial does nothing, the
; device is never closed (also not at the end of the program), and the
; read half's signal is another one.
; Offsets: u = read half (request +0, mp_SigBit +19, io_Device +58);
; lib_OpenCnt of the device at +32.
; Correct: the second OpenSerial returns 0 and leaves the open unit as
; it is; CloseSerial closes the device.
DEFTYPE .l

!TestCase{reopen_fail}
  u=OpenSerial("serial.device",0,9600,0)
  !Require{u<>0,"opened"}
  dv=Peek.l(u+58)
  sb=Peek.b(u+19)
  !Require{Peek.w(dv+32)=1,"one_user"}
  u2=OpenSerial("serial.device",0,9600,0)
  !CheckEq{u2,0,"second_open"}
  !Check{Peek.l(u)=u+38,"read_half_open"}
  !Check{Peek.b(u+19)=sb,"read_signal_kept"}
  CloseSerial 0
  !CheckEq{Peek.w(dv+32),0,"closed_users"}
!EndTestCase
