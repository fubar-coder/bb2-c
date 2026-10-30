; ristcdecrunchlib-optidec-no-checks (#833) - the S404 decruncher
; (optidec.bin, also in ristonecrackerlib) tests only at the top of its
; loop whether the output has reached the destination: a match that
; starts just above the destination writes its whole length, past the
; start, into the memory below.  Forged data in memory: unpacked length
; 2, no packed words, the first bit buffer $E800 with 15 bits: "11" + 1
; (a match of 3 bytes), "01" + 00000 (offset 0).  The 3 bytes go to
; dest + 1, dest and dest - 1; a guard byte at dest - 1 shows it.
; Correct: nothing is written below the destination.
; @rwe ristcdecrunchlib
DEFTYPE .l
m=AllocMem_(64,$10001)
If m=0 Then NPrint "no memory" : End
Poke.l m,$53343034 : Poke.l m+4,0 : Poke.l m+8,2 : Poke.l m+12,8
Poke.w m+20,0 : Poke.w m+22,$E800 : Poke.w m+24,15
d=m+40
Poke.b d-1,$5A
r=STCDecrunch(m,d)
If Peek.b(d-1)=$5A Then NPrint "guard kept" Else NPrint "guard overwritten"
FreeMem_ m,64
End
