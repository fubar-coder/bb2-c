; ristonecrackerlib-v36-calls (#1147) - STCLoad locks the file, examines
; it and opens it with OpenFromLock, a dos.library V36 function, on the
; dos.library the library opened in any version; on Kickstart 1.3 (dos
; V34) the offset lies behind dos.library's jump table: the program
; crashes at STCLoad (the session ends in a timeout).  A valid S404 file
; in RAM: (one literal byte "A", unpacked to buffer + 24), loaded to a
; buffer filled with $11.
; Correct: on every Kickstart STCLoad opens the file by its name while it
; holds the lock and unlocks the lock after that; it returns the address
; of the unpacked data and the byte "A", and the program ends normally.
; @fixed #1147
; @rwe ristonecrackerlib
; @machine a500 a500-204 a1200
; @pass timeout=30
DEFTYPE .l
NPrint "before"
m=AllocMem_(64,$10001)
b=AllocMem_(64,$10001)
If m=0 OR b=0 Then NPrint "no memory" : End
; S404, offset 24, unpacked 1, packed 4; trailer: 4 far bits, the bit
; buffer $2080 with 9 bits: "0" + $41 (a literal "A")
Poke.l m,$53343034 : Poke.l m+4,24 : Poke.l m+8,1 : Poke.l m+12,4
Poke.w m+16,4 : Poke.w m+18,$2080 : Poke.w m+20,9
If WriteFile(0,"RAM:stc-v36") Then WriteMem 0,m,22 : CloseFile 0
For k=0 To 63 : Poke.b b+k,$11 : Next
r=STCLoad("RAM:stc-v36",b)
NPrint "loaded: ",r=b+24," ",Peek.b(b+24)
KillFile "RAM:stc-v36"
FreeMem_ m,64 : FreeMem_ b,64
NPrint "after"
End
