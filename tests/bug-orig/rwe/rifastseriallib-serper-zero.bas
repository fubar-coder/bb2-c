; rifastseriallib-serper-zero (#653) - the library takes SERPER 0 for
; "not initialised": InitSerialIO with a baud rate that gives SERPER 0
; (3546895) sets the port up (the interrupts off, SERPER written), but
; then PutSerial, SetIOSpeed and FinitSerialIO do nothing.  The program
; sets 3546895 baud, then 19200 with SetIOSpeed, and asks the remote end
; (serial=on, tests/corpus/runner/serial.h) for "HELLO" with PutSerial;
; the reply is read with CheckSerial, each empty read followed by a
; VWait, at most 100.  Then InitSerialIO with baud 0, which the original
; divides by (DIVU.L: a division by zero exception).
; Correct: the library is initialised after InitSerialIO 3546895 (the
; reply comes); InitSerialIO refuses baud 0 (result 0).
; @fixed #653
; @rwe rifastseriallib
; @pass serial=on
; @pass timeout=60
DEFTYPE .l
b=AllocMem_(16,$10001)
If b=0 Then NPrint "no memory" : End
r=InitSerialIO(3546895,16,b,b)
SetIOSpeed 19200
q$=Chr$(1)+"rifastseriallib-serper-zero"+Chr$(2)+Chr$(5)+"B"
For i=1 To Len(q$) : PutSerial Asc(Mid$(q$,i,1)) : Next
a$="" : n=0
While Len(a$)<5 AND n<100
  c=CheckSerial
  If c=-1 Then VWait : n+1 Else a$+Chr$(c AND 255)
Wend
NPrint "reply: ",a$
PutSerial 4
FinitSerialIO
NPrint "baud 0"
r=InitSerialIO(0,16,b,b)
NPrint "result ",r
FinitSerialIO
FreeMem_ b,16
End
