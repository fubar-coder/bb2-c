; seriallib-closeserial-abortio (#258) - CloseSerial aborts the read
; request without WaitIO and leaves the write half's request alone: an
; asynchronous write (WriteSerialString without flag 2 = SendIO) still
; in progress stays pending after CloseSerial.  The write is held: the
; remote end on the serial port (serial=on, tests/corpus/runner/serial.h)
; answers the request S with an XOFF, and serial.device (opened with
; xON/xOFF enabled) does not send until an XON.
; The program keeps the unit open with a request of its own (io, shared
; access: SERF_SHARED $20 for both): after the original's CloseSerial it
; can still abort the pending write itself, the program ends cleanly
; (with the device closed, the reply of the write could come after the
; end of the program).
; Offsets: u = read half, u+128 = write half, its IOExtSer at u+166
; (ln_Type +8: 5 = in progress, 7 = replied; mn_Length +18; io_Error
; +31; io_SerFlags +79).
; Correct: CloseSerial aborts the write and waits for it (replied, error
; -2 = IOERR_ABORTED).  The original returns with the write pending.
; @configs clib
; @pass serial=on
DEFTYPE .l
Dim x.l(31) : io=&x(0)
u=OpenSerial("serial.device",0,9600,$20)
If u=0 Then NPrint "open failed" : End
wq=u+166
Poke.w io+18,82 : Poke.b io+79,$20 : Poke.s io+100,"serial.device"
GetReg d0,io+100 : MOVE.l d0,a0 : GetReg d0,io : MOVE.l d0,a1
MOVEQ #0,d0 : MOVEQ #0,d1 : MOVE.l 4,a6 : JSR -444(a6) : PutReg d0,oerr
If oerr Then NPrint "second open failed ",oerr : End
WriteSerialString 0,Chr$(1)+"seriallib-closeserial-abortio"+Chr$(2),2
WriteSerial 0,5 : WriteSerial 0,83
For i=1 To 50 : VWait : Next
Poke.b wq+8,5
WriteSerialString 0,"held",0
For i=1 To 10 : VWait : Next
NPrint "write ",Peek.b(wq+8)
CloseSerial 0
NPrint "after close ",Peek.b(wq+8)," error ",Peek.b(wq+31)
If Peek.b(wq+8)<>7 Then Gosub abwrite
GetReg d0,io : MOVE.l d0,a1 : MOVE.l 4,a6 : JSR -450(a6)
End

; AbortIO and WaitIO of the write request
abwrite:
  GetReg d0,wq : MOVE.l d0,a1 : MOVE.l 4,a6 : JSR -480(a6)
  GetReg d0,wq : MOVE.l d0,a1 : MOVE.l 4,a6 : JSR -474(a6)
Return
