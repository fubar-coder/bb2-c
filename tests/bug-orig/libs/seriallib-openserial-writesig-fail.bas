; seriallib-openserial-writesig-fail (#259) - if AllocSignal of the write
; half fails, OpenSerial's error path frees the read half's signal but
; does not close the device: serial.device stays open (exclusive) and
; cannot be opened again.  The test opens and closes unit 0 once (for the
; device's address: io_Device at u+58, lib_OpenCnt at +32), then leaves
; exactly one signal free (tc_SigAlloc of the task, at +18) for the next
; OpenSerial: the read half gets it, the write half's AllocSignal fails.
; Correct: OpenSerial returns 0 with the device closed again (no users),
; a later OpenSerial succeeds.
; @configs clib
DEFTYPE .l
u=OpenSerial("serial.device",0,9600,0)
If u=0 Then NPrint "open failed" : End
dv=Peek.l(u+58)
CloseSerial 0
NPrint "closed, users ",Peek.w(dv+32)
t=Peek.l(Peek.l(4)+276)
sa=Peek.l(t+18)
k=31
While k>16 AND (sa AND (1 LSL k))<>0 : k=k-1 : Wend
Poke.l t+18,-1-(1 LSL k)
r=OpenSerial("serial.device",0,9600,0)
Poke.l t+18,sa
NPrint "open with one free signal ",r
NPrint "users ",Peek.w(dv+32)
r=OpenSerial("serial.device",0,9600,0)
If r Then NPrint "open again ok" : CloseSerial 0 Else NPrint "open again failed"
End
