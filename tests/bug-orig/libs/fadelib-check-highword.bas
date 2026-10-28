; fadelib-check-highword (#135; the issue names no bug ID) - the error
; checks of FadeIn/FadeOut (fadechk0, debugger only) test only the upper
; word of the slice and palette pointers (TST (a1) / TST (a0)): a
; palette whose data lie in the lower 64 KB counts as not there.  Here
; the data pointer of palette 0 (Addr Palette +0) is poked to $8000
; (only the address matters: ASyncFade On, no DoFade, the fade only reads
; the colours) and restored after the FadeIn.
; Correct: the palette counts as set up, FadeIn passes the check, the
; program goes on.  The original reports "Not enough colours in Slice"
; (the wrong message, #330) and ends; the poked pointer then gives
; "Unable to Free Memory" at the end.
; @configs clib
; @pass runtime_errors=on
DEFTYPE .l
For i=0 To 7 : PalRGB 0,i,i*2,15-i*2,i : Next
pa=Addr Palette(0) : p=Peek.l(pa)
BLITZ
Slice 0,44,320,200,$fff8,3,8,8,320,320
Poke.l pa,$8000
ASyncFade On
FadeIn 0
Poke.l pa,p
FreeSlices
AMIGA
NPrint "faded"
End
