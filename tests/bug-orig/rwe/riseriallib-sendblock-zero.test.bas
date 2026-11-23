; riseriallib-sendblock-zero (#800) - SerialSendBlock sends the first byte
; before it looks at the length: a block of 0 bytes (or less) sends one.
; The program sends a block of 0 bytes at "QRS", then the 2 bytes "RS" to
; the remote end (serial=on, tests/corpus/runner/serial.h), which echoes
; the data bytes (request E); the echo is read with SerialRead (each
; empty read followed by a VWait, at most 50).
; Correct: the block of 0 bytes sends nothing: the echo is "RS".
DEFTYPE .l

!TestCase{sendblock_zero}
  b=AllocMem_(16,$10001)
  !Require{b<>0,"memory"}
  Poke.b b,Asc("Q") : Poke.b b+1,Asc("R") : Poke.b b+2,Asc("S")
  SerialOpen 19200
  q$=Chr$(1)+"riseriallib-sendblock-zero"+Chr$(2)
  For i=1 To Len(q$) : SerialWrite Asc(Mid$(q$,i,1)) : Next
  SerialSendBlock b,0
  SerialSendBlock b+1,2
  SerialWrite 5 : SerialWrite Asc("E")
  r$="" : n=0
  While Len(r$)<3 AND n<50
    c=SerialRead
    If c=-1 Then VWait : n+1 Else r$+Chr$(c AND $FF)
  Wend
  FreeMem_ b,16
  SerialWrite 4 : VWait 10
  !CheckStrEq{r$,"RS","echo"}
!EndTestCase
