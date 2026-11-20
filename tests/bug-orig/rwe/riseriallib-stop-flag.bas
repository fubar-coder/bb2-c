; riseriallib-stop-flag (#1162) - SerialStop outside a transfer leaves
; the stop flag set: the next SerialSendBlock stops after its first byte.
; The program calls SerialStop, then sends the block "ABCDE" to the remote
; end (serial=on, tests/corpus/runner/serial.h), which echoes the data
; bytes (request E); the echo is read with SerialRead (each empty read
; followed by a VWait, at most 50).
; Correct: every block and string command clears the flag at its start:
; the echo is "ABCDE".
; @rwe riseriallib
; @pass serial=on
DEFTYPE .l
b=AllocMem_(16,$10001)
If b=0 Then NPrint "no memory" : End
For i=0 To 4 : Poke.b b+i,65+i : Next
SerialOpen 19200
q$=Chr$(1)+"riseriallib-stop-flag"+Chr$(2)
For i=1 To Len(q$) : SerialWrite Asc(Mid$(q$,i,1)) : Next
SerialStop
SerialSendBlock b,5
SerialWrite 5 : SerialWrite Asc("E")
r$="" : n=0
While Len(r$)<5 AND n<50
  c=SerialRead
  If c=-1 Then VWait : n+1 Else r$+Chr$(c AND $FF)
Wend
NPrint "echo: ",r$
FreeMem_ b,16
SerialWrite 4 : VWait 10
End
