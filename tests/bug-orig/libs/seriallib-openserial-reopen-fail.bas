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
; @fixed #260
; @configs clib
DEFTYPE .l
u=OpenSerial("serial.device",0,9600,0)
If u=0 Then NPrint "open failed" : End
dv=Peek.l(u+58)
sb=Peek.b(u+19)
NPrint "opened, users ",Peek.w(dv+32)
u2=OpenSerial("serial.device",0,9600,0)
NPrint "second open ",u2
If Peek.l(u)=u+38 Then NPrint "read half open" Else NPrint "read half closed"
If Peek.b(u+19)=sb Then NPrint "read signal kept" Else NPrint "read signal changed"
CloseSerial 0
NPrint "closed, users ",Peek.w(dv+32)
End
