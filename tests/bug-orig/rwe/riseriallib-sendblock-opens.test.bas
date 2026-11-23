; riseriallib-sendblock-opens (#792) - the open flag is a word, but
; SerialSendBlock, SerialReceiveBlock and SerialFlush test it as a long
; (with the first instruction of SerialClose behind it: never 0), so they
; never open the port.  The program sends the request for "HELLO" to the
; remote end (serial=on, tests/corpus/runner/serial.h) with
; SerialSendBlock without SerialOpen and reads the reply with SerialRead
; (each empty read followed by a VWait, at most 100).
; Correct: SerialSendBlock opens the port (as the source means it): the
; request goes out, the reply comes.
DEFTYPE .l

!TestCase{sendblock_opens}
  q$=Chr$(1)+"riseriallib-sendblock-opens"+Chr$(2)+Chr$(5)+"B"
  b=AllocMem_(64,$10001)
  !Require{b<>0,"memory"}
  For i=1 To Len(q$) : Poke.b b+i-1,Asc(Mid$(q$,i,1)) : Next
  SerialSendBlock b,Len(q$)
  r$="" : n=0
  While Len(r$)<5 AND n<100
    c=SerialRead
    If c=-1 Then VWait : n+1 Else r$+Chr$(c AND $FF)
  Wend
  FreeMem_ b,64
  SerialWrite 4 : VWait 10
  !CheckStrEq{r$,"HELLO","reply"}
!EndTestCase
