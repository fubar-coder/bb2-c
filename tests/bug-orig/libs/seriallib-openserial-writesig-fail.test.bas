; seriallib-openserial-writesig-fail (#259) - if AllocSignal of the write
; half fails, OpenSerial's error path frees the read half's signal but
; does not close the device: serial.device stays open (exclusive) and
; cannot be opened again.  The test opens and closes unit 0 once (for the
; device's address: io_Device at u+58, lib_OpenCnt at +32), then leaves
; exactly one signal free (tc_SigAlloc of the task, at +18) for the next
; OpenSerial: the read half gets it, the write half's AllocSignal fails.
; Correct: OpenSerial returns 0 with the device closed again (no users),
; a later OpenSerial succeeds.
DEFTYPE .l

!TestCase{writesig_fail}
  u=OpenSerial("serial.device",0,9600,0)
  !Require{u<>0,"opened"}
  dv=Peek.l(u+58)
  CloseSerial 0
  !Require{Peek.w(dv+32)=0,"closed"}
  t=Peek.l(Peek.l(4)+276)
  sa=Peek.l(t+18)
  k=31
  While k>16 AND (sa AND (1 LSL k))<>0 : k=k-1 : Wend
  Poke.l t+18,-1-(1 LSL k)
  r=OpenSerial("serial.device",0,9600,0)
  Poke.l t+18,sa
  us=Peek.w(dv+32)
  !CheckEq{r,0,"one_free_signal"}
  !CheckEq{us,0,"users"}
  r=OpenSerial("serial.device",0,9600,0)
  ok=r<>0
  If r Then CloseSerial 0
  !Check{ok,"open_again"}
!EndTestCase
