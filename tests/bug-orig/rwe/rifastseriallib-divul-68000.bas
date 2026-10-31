; rifastseriallib-divul-68000 (#647) - InitSerialIO computes SERPER with
; DIVU.L, a 68020 instruction (written as data, $4C40 $1001): on a 68000
; it is an illegal instruction (the session ends in a timeout).  The
; machine is the A600 (68000).
; Correct: InitSerialIO works on a 68000 (it returns its baud argument).
; @rwe rifastseriallib
; @machine a600
; @pass timeout=30
DEFTYPE .l
NPrint "before"
r=InitSerialIO(9600,16,?inbuf,?outbuf)
NPrint "init ",r
FinitSerialIO
End
Even
inbuf: Dcb.b 64,0
outbuf: Dcb.b 64,0
