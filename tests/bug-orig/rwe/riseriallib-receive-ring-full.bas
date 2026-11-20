; riseriallib-receive-ring-full (#1160) - the receive interrupt stores
; every byte into the 256-byte ring without a test: with 256 unread bytes
; rend meets rbig and the ring counts as empty, the bytes before are
; overwritten.  The program asks the remote end (serial=on,
; tests/corpus/runner/serial.h) for 300 bytes ("0".."W" repeated), waits
; two seconds without reading, then reads what is there.
; Correct: the ring keeps the first 255 bytes (a byte that does not fit
; is dropped): 255 bytes from "0" on.
; @fixed #1160
; @rwe riseriallib
; @pass serial=on
DEFTYPE .l
SerialOpen 19200
q$=Chr$(1)+"riseriallib-receive-ring-full"+Chr$(2)+Chr$(5)+"N0300"
For i=1 To Len(q$) : SerialWrite Asc(Mid$(q$,i,1)) : Next
n=0
While n<100 : VWait : n+1 : Wend
k=0 : f=0
Repeat
  c=SerialRead
  If c<>-1
    If k=0 Then f=c AND $FF
    k+1
  EndIf
Until c=-1
NPrint k," bytes from ",Chr$(f)
SerialWrite 4 : VWait 10
End
