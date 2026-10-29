; seriallib-unit-index-128 (#252) - CloseSerial, SetSerialBuffer,
; SetSerialLens, SetSerialParams and SerialEvent compute the unit's slot
; as unit*128 instead of unit*256: unit n > 0 hits the write half of unit
; (n-1)/2 (odd n) or the read half of unit n/2 (even n).  The emulator's
; serial.device has only unit 0, so the test calls CloseSerial 1 (a unit
; that is not open) while unit 0 is open: the original closes the device
; through unit 0's write request and clears the write half.
; Offsets: u = read half (request +0, IOExtSer +38, io_Device +58),
; u+128 = write half; lib_OpenCnt of the device at +32.
; Correct: CloseSerial 1 does nothing, unit 0 stays open.
; @configs clib
DEFTYPE .l
u=OpenSerial("serial.device",0,9600,0)
If u=0 Then NPrint "open failed" : End
dv=Peek.l(u+58)
NPrint "opened, users ",Peek.w(dv+32)
CloseSerial 1
If Peek.l(u) Then NPrint "read half open" Else NPrint "read half closed"
If Peek.l(u+128) Then NPrint "write half open" Else NPrint "write half closed"
NPrint "users ",Peek.w(dv+32)
; the device is already closed: the read half is cleared so that the end
; of the program does not close it a second time
If Peek.w(dv+32)=0 Then Poke.l u,0
End
