; rifastseriallib-count-zero (#1164) - InitSerialIO baud,numbytes,...
; stores numbytes - 1 for DoSerialIO's DBRA loop: numbytes 0 becomes
; $FFFF, and DoSerialIO without argument exchanges 65536 bytes, far
; beyond buffers sized for 0 (the bytes go into the memory behind the
; input buffer).  SetIOSize 0 gives the same, as it stores its count - 1
; too.  The program (slave: DoSerialIO waits for a byte, then sends one)
; gets no data from the remote end (serial=on,
; tests/corpus/runner/serial.h): the original waits for the first of the
; 65536 bytes (the session ends in a timeout).
; Correct: a count of 0 means no transfer; DoSerialIO without argument
; returns at once after InitSerialIO baud,0,... and after SetIOSize 0,
; and the input buffer stays as it was.
; @fixed #1164
; @rwe rifastseriallib
; @pass serial=on
; @pass timeout=30
DEFTYPE .l
b=AllocMem_(64,$10001)
If b=0 Then NPrint "no memory" : End
For i=0 To 15 : Poke.b b+32+i,Asc("x") : Next
r=InitSerialIO(19200,0,b,b+32)
q$=Chr$(1)+"rifastseriallib-count-zero"+Chr$(2)
For i=1 To Len(q$) : PutSerial Asc(Mid$(q$,i,1)) : Next
DoSerialIO
NPrint "InitSerialIO 0: ",Peek.b(b)
SetIOSize 4
SetIOSize 0
DoSerialIO
NPrint "SetIOSize 0: ",Peek.b(b)
PutSerial 4
FinitSerialIO
FreeMem_ b,64
End
