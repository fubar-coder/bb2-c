; riseriallib-receiveblock-one (#799) - SerialReceiveBlock counts the
; bytes after the first one down to 0: with a length of 1 the count goes
; below 0 and it receives without end, behind the buffer (the session
; ends in a timeout).  The program asks the remote end (serial=on,
; tests/corpus/runner/serial.h) for "Z" and receives a block of 1 into a
; cleared buffer.
; Correct: one byte "Z", the byte behind the buffer stays 0.
; @rwe riseriallib
; @pass serial=on
; @pass timeout=30
DEFTYPE .l
b=AllocMem_(16,$10001)
If b=0 Then NPrint "no memory" : End
SerialOpen 19200
q$=Chr$(1)+"riseriallib-receiveblock-one"+Chr$(2)+Chr$(5)+"A"
For i=1 To Len(q$) : SerialWrite Asc(Mid$(q$,i,1)) : Next
SerialReceiveBlock b,1
NPrint Chr$(Peek.b(b))," ",Peek.b(b+1)
FreeMem_ b,16
SerialWrite 4 : VWait 10
End
