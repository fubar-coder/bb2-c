; seriallib-readserialstring-unbounded (#256) - ReadSerialString reads
; every byte waiting (SDCMD_QUERY's io_Actual) into the string buffer,
; without a limit: more than the buffer holds overwrites the memory
; behind it.  The string buffer is 1000000 bytes (tedstub strbuf=; that
; large, it is allocated from the big free chunk of the fast memory, so
; the memory behind it is free); the program's first statements find it
; among the globals (a5-32768...: the pointer to the "QQQQQQQQ" a string
; expression has just left there) and allocate the 4096 bytes behind it
; as a guard (exec AllocAbs, filled with "G"), where the overwrite does
; no harm.  ReadSerialString then comes behind 999500 bytes of the same
; expression (500 bytes of room left; only its length is used: no
; allocation); the serial buffer is 4096 (SetSerialBuffer), the remote
; end on the serial port (serial=on, tests/corpus/runner/serial.h)
; sends 1000 bytes (request N).
; Offsets: u = read half, its IOExtSer at u+38 (io_Command +66,
; io_Actual +70).
; Correct: ReadSerialString returns at most what fits in the buffer (the
; expression is at most 1000000 bytes long), the guard stays intact.
; @configs clib
; @pass serial=on
; @pass jobline=strbuf=1000000
DEFTYPE .l
MOVE.l a5,d0 : PutReg d0,g5
n=Len(String$("Q",8))
sb=0
For i=0 To 252 Step 4
  p=Peek.l(g5-32768+i)
  If sb=0 AND p>0 AND (p AND 7)=0
    If Peek.l(p)=$51515151 AND Peek.l(p+4)=$51515151 Then sb=p
  EndIf
Next
If sb=0 Then NPrint "no string buffer" : End
GetReg d0,sb+1000000 : MOVE.l d0,a1 : MOVE.l #4096,d0 : MOVE.l 4,a6 : JSR -204(a6) : PutReg d0,gd
If gd=0 Then NPrint "behind the buffer: in use" : End
For i=0 To 4092 Step 4 : Poke.l gd+i,$47474747 : Next
u=OpenSerial("serial.device",0,19200,$80)
If u=0 Then NPrint "open failed" : End
rq=u+38
SetSerialBuffer 0,4096
WriteSerialString 0,Chr$(1)+"seriallib-readserialstring-unbounded"+Chr$(2),2
WriteSerialString 0,Chr$(5)+"N1000",2
; wait (at most 5 s) until the 1000 bytes are there
Gosub qry : tk=0
While qn<1000 AND tk<250 : VWait : tk=tk+1 : Gosub qry : Wend
NPrint "waiting ",qn
n=Len(String$("a",999500)+ReadSerialString(0))
If n<=1000000 Then NPrint "fits" Else NPrint "length ",n
k=0
For i=0 To 4092 Step 4 : If Peek.l(gd+i)<>$47474747 Then k=k+4
Next
If k Then NPrint "guard: ",k," bytes overwritten" Else NPrint "guard intact"
WriteSerial 0,4
CloseSerial 0
GetReg d0,gd : MOVE.l d0,a1 : MOVE.l #4096,d0 : MOVE.l 4,a6 : JSR -210(a6)
End

; SDCMD_QUERY through the read request -> qn
qry:
  Poke.w u+66,9
  GetReg d0,rq : MOVE.l d0,a1 : MOVE.l 4,a6 : JSR -456(a6)
  qn=Peek.l(u+70)
Return
