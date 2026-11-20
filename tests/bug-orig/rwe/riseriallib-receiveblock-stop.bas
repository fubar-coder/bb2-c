; riseriallib-receiveblock-stop (#803) - SerialStop during
; SerialReceiveBlock ends it through the abort code of SerialSendBlock,
; which pops the saved d7 - here the return address - into d7, so it
; returns to the long above it on the stack (the session ends in a
; timeout).  The program asks the remote end (serial=on,
; tests/corpus/runner/serial.h) for 2000 bytes, calls SerialStop in a
; vertical blank interrupt routine (SetInt 5) and receives a block of
; 3000.
; Correct: SerialReceiveBlock returns to the program after the byte that
; follows the SerialStop.
; @rwe riseriallib
; @pass serial=on
; @pass timeout=30
DEFTYPE .l
b=AllocMem_(3000,$10001)
If b=0 Then NPrint "no memory" : End
SerialOpen 19200
q$=Chr$(1)+"riseriallib-receiveblock-stop"+Chr$(2)+Chr$(5)+"N2000"
SetInt 5
  SerialStop
End SetInt
For i=1 To Len(q$) : SerialWrite Asc(Mid$(q$,i,1)) : Next
SerialReceiveBlock b,3000
ClrInt 5
NPrint "returned"
FreeMem_ b,3000
SerialWrite 4 : VWait 10
End
