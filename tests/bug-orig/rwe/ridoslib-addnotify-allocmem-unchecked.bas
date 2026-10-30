; ridoslib-addnotify-allocmem-unchecked (#619) - AddNotify copies the file
; name into AllocMem(length + 1) and builds the NotifyRequest in
; AllocMem(48) without testing either result.  The program runs through
; inject, which fails the first allocation of 4 bytes (the copy of the
; 3-character name "a/b"): the name is copied to address 0 (4 bytes, the
; ExecBase pointer at 4 stays).  The long at address 0 holds a marker
; (put back at once); "a/b" does not exist, so StartNotify fails and
; nothing is left to free at the end.
; Correct: AddNotify fails (0) without writing to address 0.
; @rwe ridoslib
; @pass runwrap=@INJECT@|allocfail=4-4/1
DEFTYPE .l
o=Peek.l(0)
Poke.l 0,$12345678
r=AddNotify(1,"a/b")
m=Peek.l(0)
Poke.l 0,o
NPrint "result ",r
If m=$12345678 Then NPrint "address 0 kept" Else NPrint "address 0 written"
End
