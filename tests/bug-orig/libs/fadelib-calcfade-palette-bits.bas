; fadelib-calcfade-palette-bits (#134) - calcfade reads every colour
; component with MOVE.b into d0 and shifts the word (LSR #4,d0) without
; clearing d0 first; d0.w comes in as the palette's object offset
; (palette# * 128), so its bits 8-11 get into the red fraction and bits
; 12-15 into the green fraction of the first colour of the range.
; Here: palette 2 (d0 = $100) with all 8 colours 15,15,15, FadeIn 2 with
; ASyncFade On in Blitz mode, colour 0 of the copper list after each of
; the 16 DoFade steps (collected in Blitz mode, printed after AMIGA, as
; in t123_blitzfade).
; Correct: all three components rise together: 000 111 222 ... FFF.  The
; original: the red fraction is $1F instead of $0F, so red runs ahead
; (100 311 ...); the last step is right again.
; @fixed #134
; @configs clib
DEFTYPE .l
Dim v.l(16)
For i=0 To 7 : PalRGB 2,i,15,15,15 : Next
BLITZ
Slice 0,44,320,200,$fff8,3,8,8,320,320
cp=Peek.l(Addr Slice(0)+18)
ASyncFade On
FadeIn 2
For k=1 To 16 : DoFade : v(k)=Peek.w(cp+2) : Next
FreeSlices
AMIGA
Print "colour 0"
For k=1 To 16 : Print " ",Right$(Hex$(v(k)),3) : Next
NPrint ""
End
