; risoundlib-cludgesound-frees-decoded (#945) - CludgeSound frees the old
; sample of the sound object (memlib globfree) without looking at
; audiolib's "cL" mark (+14): after a DecodeSound of an 8SVX file in chip
; memory the object's sample is the file's BODY data itself, which memlib
; never allocated.  memlib does not find the block and raises its runtime
; error "Unable to Free Memory" (TRAP #0, runtime error checking on: the
; debugger's requester).  The 8SVX file is built in chip memory: a VHDR
; of the usual length 20 with samplesPerSec 8000, a BODY of 4 bytes.
; Correct: no runtime error, CludgeSound returns -1, the program goes on.
; @rwe risoundlib
; @pass runtime_errors=on
DEFTYPE .l
m=AllocMem_(64,$10002)
If m=0 Then NPrint "no memory" : End
Poke.l m,$464F524D : Poke.l m+4,44 : Poke.l m+8,$38535658
Poke.l m+12,$56484452 : Poke.l m+16,20
Poke.l m+20,4 : Poke.w m+32,8000 : Poke.b m+34,1 : Poke.l m+36,$10000
Poke.l m+40,$424F4459 : Poke.l m+44,4 : Poke.l m+48,$01020304
DecodeSound 0,m
NPrint "decoded"
r=CludgeSound(0,m)
NPrint "cludged ",r
FreeMem_ m,64
End
