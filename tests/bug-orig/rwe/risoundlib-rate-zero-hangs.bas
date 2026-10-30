; risoundlib-rate-zero-hangs (#830) - CludgeSound computes the period
; 3579545 / rate by a subtraction loop that ends when the rest drops
; below the rate: with a rate of 0 it never ends, the program hangs (the
; session ends in a timeout).  The 8SVX data are built in memory: a VHDR
; of the usual length 20 with samplesPerSec 0, a BODY of 4 bytes.
; Correct: CludgeSound returns, the program goes on.
; @rwe risoundlib
DEFTYPE .l
m=AllocMem_(64,$10001)
If m=0 Then NPrint "no memory" : End
Poke.l m,$464F524D : Poke.l m+4,44 : Poke.l m+8,$38535658
Poke.l m+12,$56484452 : Poke.l m+16,20
Poke.l m+20,4 : Poke.w m+32,0 : Poke.b m+34,1 : Poke.l m+36,$10000
Poke.l m+40,$424F4459 : Poke.l m+44,4 : Poke.l m+48,$01020304
NPrint "before"
r=CludgeSound(0,m)
NPrint "after"
FreeMem_ m,64
End
