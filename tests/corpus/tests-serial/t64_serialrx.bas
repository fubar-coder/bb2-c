; t64_serialrx - seriallib with a remote end: serial.device unit 0 of the
; emulator is connected (Amiberry serial_port=TCP://...) to the remote end
; of tests/corpus/runner (serial.h).  Only run with serial=on (ctest
; corpus_serial, corpus_serial_runerr): the other passes have no remote end.
; Protocol: Chr$(1)+name+Chr$(2) starts a session, Chr$(5)+code asks for
; a canned reply (A "Z", B "HELLO", C "0123456789ABCDEF", D bytes
; $80-$87, E echo of the data since the last request, L 100 bytes
; "0".."W" repeated, W "w" after 0.1 s), Chr$(4) ends the session.
; Nothing blocks for ever: blocking reads (ReadSerial with wait flag,
; ReadSerialMem) run under a timer.device watchdog (3 s; a software
; interrupt AbortIO's the request), everything else polls SDCMD_QUERY or
; the reply type of a request for at most 3 s; a missing answer prints
; TIMEOUT and later waits are skipped.
; Offsets: u = read half (request, port +4, IOExtSer +38), u+128 = write
; half; IOExtSer: +8 ln_Type, +28 command, +31 error, +32 actual,
; +36 length, +40 data, +52 rbuflen, +60 baud, +68 termarray, +76 lens,
; +79 serflags.
DEFTYPE .l
Dim m.l(31) : b=&m(0)
Dim hm.l(100) : hb=&hm(0)
lim=150
Gosub wdsetup
NPrint "timer ",herr
u=OpenSerial("serial.device",0,9600,$80)
If u=0 Then NPrint "open failed" : End
rq=u+38 : wq=u+166
NPrint "open baud ",Peek.l(u+98)," flags ",Hex$(Peek.b(u+117) AND 255)," err ",Peek.b(u+69)
smask=(1 LSL (Peek.b(u+19) AND 31)) | (1 LSL (Peek.b(u+147) AND 31))
Gosub qry
If qn>0 Then NPrint "stale ",qn : s$=ReadSerialString(0)
SetSerialBuffer 0,1024
NPrint "buffer ",Peek.l(u+90)," err ",Peek.b(u+69)
; session start (TrueString + DoIO)
WriteSerialString 0,Chr$(1)+"t64_serialrx"+Chr$(2),2
NPrint "start err ",Peek.b(u+128+69)," len ",Peek.l(u+128+74)," actual ",Peek.l(u+128+70)

; nothing waiting: -1 / empty string; flag 1 does not wait (BUG(orig))
NPrint "empty ",ReadSerial(0)," ",ReadSerial(0,0)," ",ReadSerial(0,1)
s$=ReadSerialString(0) : NPrint "emptystr [",s$,"] ",Len(s$)
NPrint "event ",SerialEvent(0)

; A: one byte, ReadSerial without wait once it is there
cd=65 : Gosub req : nd=1 : lb$="A" : Gosub waitq
NPrint "A ",ReadSerial(0)," ",ReadSerial(0)

; W: reply after 0.1 s, ReadSerial with wait flag (2) blocks until then
cd=87 : Gosub req : r=-2
If herr=0 Then tg=rq : Gosub wdstart : r=ReadSerial(0,2) : Gosub wdstop
If wdf Then NPrint "TIMEOUT W"
NPrint "W ",r," err ",Peek.b(u+69)," actual ",Peek.l(u+70)

; B: "HELLO" - wait flag with data waiting, flag 1, ReadSerialString
cd=66 : Gosub req : nd=5 : lb$="B" : Gosub waitq
r=ReadSerial(0,2) : r2=ReadSerial(0,1)
s$=ReadSerialString(0)
NPrint "B ",r," ",r2," [",s$,"] ",Len(s$)," ",ReadSerial(0)

; C: 16 bytes with ReadSerialString
cd=67 : Gosub req : nd=16 : lb$="C" : Gosub waitq
s$=ReadSerialString(0) : Gosub qry
NPrint "C [",s$,"] ",Len(s$)," left ",qn

; C again: ReadSerialMem 10 bytes (blocks, through the write request),
; the rest with ReadSerialString
For i=0 To 31 : m(i)=0 : Next
cd=67 : Gosub req : Gosub mem10
lb$="C rest" : nd=6 : Gosub waitq
s$=ReadSerialString(0)
NPrint "mem rest [",s$,"] ",Len(s$)

; D: bytes $80-$87
cd=68 : Gosub req : nd=8 : lb$="D" : Gosub waitq
Print "D"
For i=1 To 8 : Print " ",ReadSerial(0) : Next
NPrint " ",ReadSerial(0)

; E: echo of WriteSerialString (TrueString) + WriteSerialMem
WriteSerialString 0,"Hello, serial",2
Poke.l b+64,$5758595A
WriteSerialMem 0,b+64,4
NPrint "mem write err ",Peek.b(u+128+69)," len ",Peek.l(u+128+74)," actual ",Peek.l(u+128+70)
cd=69 : Gosub req : nd=17 : lb$="E" : Gosub waitq
s$=ReadSerialString(0)
NPrint "E [",s$,"] ",Len(s$)

; SerialEvent: Wait after an asynchronous write (WriteSerialString
; without flag 2 = SendIO) -> 1
Gosub clrsig
Poke.b wq+8,5
WriteSerialString 0,"async",0
ar=wq : lb$="async" : Gosub waitrp
If ok Then sg=Wait : NPrint "event write ",SerialEvent(0) : Gosub waitio
; two argument form: address of a null terminated string
Poke.l b+80,$61646472 : Poke.l b+84,0
Gosub clrsig
Poke.b wq+8,5
WriteSerialString 0,b+80
ar=wq : lb$="addr" : Gosub waitrp
If ok Then sg=Wait : NPrint "event addr ",SerialEvent(0) : Gosub waitio
cd=69 : Gosub req : nd=9 : lb$="E2" : Gosub waitq
s$=ReadSerialString(0)
NPrint "E2 [",s$,"] ",Len(s$)

; SerialEvent: Wait after an asynchronous read of our own -> 2
cd=65 : Gosub req : nd=1 : lb$="A2" : Gosub waitq
Gosub clrsig
Poke.b b+96,0
Poke.w u+66,2 : Poke.l u+74,1 : Poke.l u+78,b+96 : Poke.b rq+8,5
ar=rq : Gosub sendio
lb$="aread" : Gosub waitrp
If ok Then sg=Wait : NPrint "event read ",SerialEvent(0)," ",Peek.b(b+96) : Gosub waitio

; both signals -> 2 wins
cd=65 : Gosub req : nd=1 : lb$="A3" : Gosub waitq
Gosub clrsig
Poke.b b+96,0
Poke.b wq+8,5
WriteSerialString 0,b+80
Poke.w u+66,2 : Poke.l u+74,1 : Poke.l u+78,b+96 : Poke.b rq+8,5
ar=rq : Gosub sendio
lb$="both read" : Gosub waitrp : ok2=ok
ar=wq : lb$="both write" : Gosub waitrp
If ok AND ok2 Then sg=Wait : NPrint "event both ",SerialEvent(0)," ",Peek.b(b+96)
If ok Then ar=wq : Gosub waitio
If ok2 Then ar=rq : Gosub waitio
cd=69 : Gosub req : nd=4 : lb$="E3" : Gosub waitq
s$=ReadSerialString(0)
NPrint "E3 [",s$,"] ",Len(s$)

; SetSerialLens: 7 data bits
SetSerialLens 0,7,7,1
NPrint "lens ",Peek.b(u+114)," ",Peek.b(u+115)," ",Peek.b(u+116)," err ",Peek.b(u+69)
WriteSerialString 0,"ab",2
cd=69 : Gosub req : nd=2 : lb$="E7" : Gosub waitq
; two ReadSerial in one statement differ: the original leaves a3 on its
; byte buffer, the next string part is built there (DIFF in seriallib.c)
r=ReadSerial(0) : r2=ReadSerial(0)
NPrint "E7 ",r," ",r2
cd=68 : Gosub req : nd=8 : lb$="D7" : Gosub waitq
Print "D7"
For i=1 To 8 : Print " ",ReadSerial(0) : Next
NPrint ""
SetSerialLens 0,8,8,1
NPrint "lens ",Peek.b(u+114)," ",Peek.b(u+115)," ",Peek.b(u+116)," err ",Peek.b(u+69)

; SetSerialParams: other baud rate, EOF mode with terminator "5"
Poke.l u+98,19200
SetSerialParams 0
NPrint "params ",Peek.l(u+98)," err ",Peek.b(u+69)
cd=65 : Gosub req : nd=1 : lb$="A4" : Gosub waitq
NPrint "A4 ",ReadSerial(0)
Poke.b u+117,$C0
For i=0 To 7 : Poke.b u+106+i,$35 : Next
SetSerialParams 0
NPrint "eofmode err ",Peek.b(u+69)
For i=0 To 31 : m(i)=0 : Next
cd=67 : Gosub req : Gosub mem16
Poke.b u+117,$80
SetSerialParams 0
lb$="eof rest" : nd=16-k : Gosub waitq
s$=ReadSerialString(0)
NPrint "eof rest [",s$,"] ",Len(s$)," err ",Peek.b(u+69)

; SetSerialBuffer: 64 bytes, 100 sent -> overflow
SetSerialBuffer 0,64
NPrint "buffer ",Peek.l(u+90)," err ",Peek.b(u+69)
cd=76 : Gosub req : nd=64 : lb$="L64" : Gosub waitq
For i=1 To 25 : VWait : Next
Gosub qry : q1=qn
s$=ReadSerialString(0)
; the read fails (SerErr_BufOverflow): s$ holds no received data
NPrint "L64 waiting ",q1," got ",Len(s$)," err ",Peek.b(u+69)
For i=1 To 25 : VWait : Next
Gosub qry : q2=qn
s$=ReadSerialString(0)
NPrint "L64 rest ",q2," got ",Len(s$)," ",Left$(s$,4)," err ",Peek.b(u+69)
SetSerialBuffer 0,1024
NPrint "buffer ",Peek.l(u+90)," err ",Peek.b(u+69)
cd=76 : Gosub req : nd=100 : lb$="L" : Gosub waitq
s$=ReadSerialString(0)
NPrint "L got ",Len(s$)," ",Left$(s$,4)," ",Right$(s$,4)," err ",Peek.b(u+69)

; end of session, close
WriteSerial 0,4
CloseSerial 0
NPrint "closed ",Peek.l(u)," ",Peek.l(u+128)
If herr=0 Then GetReg d0,t1 : MOVE.l d0,a1 : MOVE.l 4,a6 : JSR -450(a6)
NPrint "watchdog ",Peek.l(wd+8)," dead ",dead
NPrint "done"
End

; ---------------------------------------------------------------------
; request reply cd
req:
  WriteSerial 0,5 : WriteSerial 0,cd
Return

; SDCMD_QUERY through the read request -> qn
qry:
  Poke.w u+66,9
  GetReg d0,rq : MOVE.l d0,a1 : MOVE.l 4,a6 : JSR -456(a6)
  qn=Peek.l(u+70)
Return

; wait (bounded) until nd bytes are waiting
waitq:
  Gosub qry : tk=0
  While qn<nd AND tk<lim : VWait : tk=tk+1 : Gosub qry : Wend
  ok=1
  If qn<nd Then ok=0 : dead=1 : lim=0 : NPrint "TIMEOUT ",lb$," ",qn
Return

; wait (bounded) until request ar is replied; else abort it
waitrp:
  tk=0
  While (Peek.b(ar+8) AND 255)<>7 AND tk<lim : VWait : tk=tk+1 : Wend
  ok=1
  If (Peek.b(ar+8) AND 255)<>7 Then ok=0 : dead=1 : lim=0 : NPrint "TIMEOUT ",lb$ : Gosub abio
Return

sendio:
  GetReg d0,ar : MOVE.l d0,a1 : MOVE.l 4,a6 : JSR -462(a6)
Return

waitio:
  GetReg d0,ar : MOVE.l d0,a1 : MOVE.l 4,a6 : JSR -474(a6)
Return

abio:
  GetReg d0,ar : MOVE.l d0,a1 : MOVE.l 4,a6 : JSR -480(a6)
  Gosub waitio
Return

; clear both signals of the unit (SetSignal 0,smask)
clrsig:
  GetReg d0,smask : MOVE.l d0,d1 : MOVEQ #0,d0 : MOVE.l 4,a6 : JSR -306(a6)
Return

; ReadSerialMem of 10 bytes under the watchdog
mem10:
  If herr=0 Then tg=wq : Gosub wdstart : ReadSerialMem 0,b,10 : Gosub wdstop
  If wdf Then NPrint "TIMEOUT mem"
  s$="" : For i=0 To 9 : s$=s$+Chr$(Peek.b(b+i) AND 255) : Next
  NPrint "mem [",s$,"] err ",Peek.b(u+128+69)," len ",Peek.l(u+128+74)," actual ",Peek.l(u+128+70)
Return

; ReadSerialMem of 16 bytes (EOF mode) under the watchdog; k = actual
mem16:
  If herr=0 Then tg=wq : Gosub wdstart : ReadSerialMem 0,b,16 : Gosub wdstop
  If wdf Then NPrint "TIMEOUT eof"
  k=Peek.l(u+128+70) : If k<0 OR k>16 Then k=0
  s$="" : For i=0 To 15 : If Peek.b(b+i) Then s$=s$+Chr$(Peek.b(b+i) AND 255)
  Next
  NPrint "eof [",s$,"] err ",Peek.b(u+128+69)," actual ",k
Return

; ---------------------------------------------------------------------
; watchdog: a timer.device request (3 s) replies to a port with a
; software interrupt (PA_SOFTINT).  Its code (poked below; is_Data = wd):
;   MOVEM.l a2/a6,-(a7) : MOVEA.l a1,a2 : MOVEA.l 4.w,a6
;   MOVEA.l (a2),a0 : JSR GetMsg(a6) : TST.l d0 : BEQ.s done
;   TST.l 12(a2) : BEQ.s done              ; armed?
;   MOVEA.l 4(a2),a1 : JSR AbortIO(a6)     ; abort the guarded request
;   ADDQ.l #1,8(a2)                        ; count
;   done: MOVEM.l (a7)+,a2/a6 : RTS
; wd: +0 timer port, +4 guarded request, +8 count, +12 armed.
; wdf<>0 after wdstop: the request had to be aborted (= it would hang).
wdsetup:
  hx$="48E7,0022,2449,2C78,0004,2052,4EAE,FE8C,4A80,6712,4AAA,000C,670C,226A,0004,4EAE,FE20,52AA,0008,4CDF,4400,4E75"
  ha=hb
  While hx$<>""
    k=Instr(hx$,",")
    If k=0 Then h$=hx$ : hx$="" Else h$=Left$(hx$,k-1) : hx$=Mid$(hx$,k+1)
    Poke.w ha,Val("$"+h$) : ha=ha+2
  Wend
  MOVE.l 4,a6 : JSR -636(a6)
  wd=hb+48 : tp=hb+64 : ti=hb+100 : t1=hb+128
  Poke.b tp+8,4 : Poke.b tp+14,1 : Poke.l tp+16,ti
  Poke.l tp+20,tp+24 : Poke.l tp+24,0 : Poke.l tp+28,tp+20
  Poke.b ti+8,2 : Poke.l ti+14,wd : Poke.l ti+18,hb
  Poke.l wd,tp
  Poke.s hb+224,"timer.device"
  Poke.l t1+14,tp : Poke.w t1+18,40
  GetReg d0,hb+224 : MOVE.l d0,a0 : GetReg d0,t1 : MOVE.l d0,a1
  MOVEQ #1,d0 : MOVEQ #0,d1 : MOVE.l 4,a6 : JSR -444(a6) : PutReg d0,herr
Return

wdstart:
  Poke.l wd+4,tg : Poke.l wd+12,1 : wf0=Peek.l(wd+8)
  Poke.b t1+8,5 : Poke.w t1+28,9 : Poke.l t1+32,3 : Poke.l t1+36,0
  GetReg d0,t1 : MOVE.l d0,a1 : MOVE.l 4,a6 : JSR -462(a6)
Return

wdstop:
  Poke.l wd+12,0
  GetReg d0,t1 : MOVE.l d0,a1 : MOVE.l 4,a6 : JSR -480(a6)
  tk=0
  While (Peek.b(t1+8) AND 255)<>7 AND tk<250 : VWait : tk=tk+1 : Wend
  wdf=Peek.l(wd+8)-wf0
Return
