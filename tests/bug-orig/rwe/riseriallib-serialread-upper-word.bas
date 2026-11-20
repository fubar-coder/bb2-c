; riseriallib-serialread-upper-word (#797) - SerialRead (#long result)
; replaces only the low word of d0 (MOVE.w rbig,d0, then the byte into
; its low byte): the upper word of d0 at the call stays in the result.
; The program asks the remote end (serial=on,
; tests/corpus/runner/serial.h) for "Z", waits for it and calls
; SerialRead with $1234 in the upper word of d0.
; Correct: SerialRead returns the byte alone: 90.
; @rwe riseriallib
; @pass serial=on
DEFTYPE .l
SerialOpen 19200
q$=Chr$(1)+"riseriallib-serialread-upper-word"+Chr$(2)+Chr$(5)+"A"
For i=1 To Len(q$) : SerialWrite Asc(Mid$(q$,i,1)) : Next
n=0
While SerialGot=0 AND n<100 : VWait : n+1 : Wend
MOVE.l #$12340000,d0 : c=SerialRead
NPrint c
SerialWrite 4 : VWait 10
End
