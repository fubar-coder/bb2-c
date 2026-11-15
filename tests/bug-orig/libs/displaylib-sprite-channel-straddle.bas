; displaylib-sprite-channel-straddle (#414) - DisplaySprite checks the
; channels of a sprite against twice the sprites of the CopList when it
; has a CustomSprites bank, but chooses the sprite moves to write (the
; normal bank or the CustomSprites bank) by the first channel only: an
; attached sprite (2 channels) on channel 7 of a CopList with 8 sprites
; and CustomSprites writes its second channel 16 bytes behind the normal
; sprite moves, over the BPLCON moves.
; Amiga mode (the list is built in memory only, as in t164_display; the
; sprite object is poked as there).  The 32 bytes behind the 8 normal
; sprite moves (+12 of the CopList, 16 bytes per channel) before and
; after DisplaySprite; SetErr catches a runtime error (also without the
; debugger: the error routines of displaylib are in the normal code).
; Correct: the two banks are different screen areas, so a sprite whose
; channels do not fit into the bank of its first channel is the runtime
; error "Not Enough Sprite Channels" (as without CustomSprites); the
; moves behind the normal bank are not changed.
; @fixed #414
; @configs clib
DEFTYPE .l
Dim sd.l(63) : For i=0 To 63 : sd(i)=i*$01010101 : Next
s=Addr Sprite(0)
Poke.l s,&sd(0) : Poke.w s+4,10 : Poke.w s+6,2 : Poke.w s+8,-1
Poke.w s+10,44 : Poke.w s+12,2 : Poke.w s+14,1
InitCopList 0,44,32,$10004,8,32,80
CustomSprites 0,0,14,8
c=Addr CopList(0)
p=Peek.l(c+12)+128
Dim o.l(7)
For i=0 To 7 : o(i)=Peek.l(p+i*4) : Next
SetErr
  NPrint "error"
  Gosub kept
  End
End SetErr
DisplaySprite 0,0,50,20,7
NPrint "no error"
Gosub kept
End

kept:
  k=0
  For i=0 To 7
    If Peek.l(p+i*4)=o(i) Then k=k+1
  Next
  NPrint "moves behind the bank kept ",k," of 8"
Return
