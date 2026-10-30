; rifastseriallib-getserial-result (#649) - GetSerial address,bytes is
; declared with a #byte result like the form without arguments, but
; never sets d0: the result is the low byte of its address argument.
; The library drives the serial hardware itself (SERDAT/SERDATR); the
; remote end on the emulated serial port (serial=on,
; tests/corpus/runner/serial.h) answers request B with "HELLO".  The
; buffer comes from AllocMem (low byte a multiple of 8, never "O"); the
; program prints "address" for a result equal to the low byte of it.
; Correct: GetSerial returns the last byte received ("O", 79).
; @rwe rifastseriallib
; @pass serial=on
DEFTYPE .l
b=AllocMem_(16,$10001)
If b=0 Then NPrint "no memory" : End
r=InitSerialIO(19200,16,b,b)
q$=Chr$(1)+"rifastseriallib-getserial-result"+Chr$(2)+Chr$(5)+"B"
For i=1 To Len(q$) : PutSerial Asc(Mid$(q$,i,1)) : Next
g.b=GetSerial(b,5)
For i=0 To 4 : Print Chr$(Peek.b(b+i)) : Next
NPrint ""
If (g AND $FF)=(b AND $FF) Then NPrint "address" Else NPrint g
PutSerial 4
FinitSerialIO
FreeMem_ b,16
End
