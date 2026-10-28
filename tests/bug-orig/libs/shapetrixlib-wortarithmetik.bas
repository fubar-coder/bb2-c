; shapetrixlib-wortarithmetik (#274) - XFlip/YFlip/Rotate/Scale add row
; and plane steps to address registers as words (sign extended) and keep
; sizes as the low words of MULU.
; Here: YFlip of a 512x520 shape with 1 plane (33280 bytes per plane, 32K
; or more): the bottom row pointer (data + bytes per plane, ADDA.w) lands
; below the top row, the loop ends at once and nothing is flipped.  One
; plane only, so the wrong plane step reads and writes nothing.
; Correct: the shape is flipped - the marker byte of the top row ($AB)
; ends up in the bottom row.
; @configs clib
DEFTYPE .l
AutoCookie Off
InitShape 0,512,520,1
a=Addr Shape(0)
d=Peek.l(a+14)
NPrint "shape ",Peek.w(a)," ",Peek.w(a+2)," per plane ",Peek.w(a+22) AND $FFFF
Poke.b d,$AB
YFlip 0
NPrint "top ",Right$(Hex$(Peek.b(d) AND 255),2)," bottom ",Right$(Hex$(Peek.b(d+519*64) AND 255),2)
End
