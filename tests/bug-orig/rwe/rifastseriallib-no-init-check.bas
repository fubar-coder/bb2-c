; rifastseriallib-no-init-check (#652) - only FinitSerialIO, InitSerialIO,
; PutSerial and SetIOSpeed test whether the library is initialised;
; DoSerialIO, GetSerial, SyncSerial, SyncPut, SyncGet and CheckSerial
; run on the port without it, DoSerialIO with the buffers 0: it reads
; into and sends from address 0.  The program sets up the port itself
; (SERPER for 19200 baud, the RBF interrupt off, as InitSerialIO does)
; and asks the remote end (serial=on, tests/corpus/runner/serial.h) for
; "HELLO"; then DoSerialIO 0 (one byte, slave: read, then write) without
; InitSerialIO.  The long at address 0 holds a marker (put back at the
; end).
; Correct: without InitSerialIO DoSerialIO does nothing (the marker
; stays).
; @rwe rifastseriallib
; @pass serial=on
DEFTYPE .l
ie.w=Peek.w($DFF01C)
Poke.w $DFF09A,$0800
Poke.w $DFF032,183
q$=Chr$(1)+"rifastseriallib-no-init-check"+Chr$(2)+Chr$(5)+"B"
For i=1 To Len(q$)
  While (Peek.w($DFF018) AND $2000)=0 : Wend
  Poke.w $DFF030,$100 OR Asc(Mid$(q$,i,1))
Next
o=Peek.l(0)
Poke.l 0,$12345678
DoSerialIO 0
m=Peek.l(0)
Poke.l 0,o
If m=$12345678 Then NPrint "address 0 kept" Else NPrint "address 0 written"
While (Peek.w($DFF018) AND $2000)=0 : Wend
Poke.w $DFF030,$104
If ie AND $0800 Then Poke.w $DFF09A,$8800
End
