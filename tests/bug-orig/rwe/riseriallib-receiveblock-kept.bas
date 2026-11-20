; riseriallib-receiveblock-kept (#798) - SerialReceiveBlock first throws
; away what is in the receive buffer, then waits for the block: bytes that
; arrived before the call are lost.  The program asks the remote end
; (serial=on, tests/corpus/runner/serial.h) for "HELLO", waits until the
; five bytes are there, and receives a block of 5: the original throws
; them away and waits for bytes that never come (the session ends in a
; timeout).
; Correct (decided: the discard loop goes, a program calls SerialFlush
; before if it wants an empty buffer): the block is "HELLO".
; @rwe riseriallib
; @pass serial=on
; @pass timeout=30
DEFTYPE .l
b=AllocMem_(16,$10001)
If b=0 Then NPrint "no memory" : End
SerialOpen 19200
q$=Chr$(1)+"riseriallib-receiveblock-kept"+Chr$(2)+Chr$(5)+"B"
For i=1 To Len(q$) : SerialWrite Asc(Mid$(q$,i,1)) : Next
n=0
While n<50 : VWait : n+1 : Wend
SerialReceiveBlock b,5
For i=0 To 4 : Print Chr$(Peek.b(b+i)) : Next
NPrint ""
FreeMem_ b,16
SerialWrite 4 : VWait 10
End
