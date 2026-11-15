; displaylib-spriteoff-custom-bank (#1051) - DisplaySprite of a
; sprite completely above or below the display gives its channels the
; blank sprite (spriteoff), but spriteoff always writes into the normal
; sprite moves at channel x 16: for a channel of the CustomSprites bank
; (channel >= the sprites of the CopList) that is behind the normal
; moves, over the BPLCON moves, and the custom channel keeps the sprite.
; Amiga mode (the list is built in memory only, as in t164_display; the
; sprite object is poked as there, one channel): a CopList with 8
; sprites and CustomSprites; the sprite on channel 9 (channel 1 of the
; custom bank) visible, then below the display; the 32 bytes behind the
; 8 normal sprite moves (+12 of the CopList, 16 bytes per channel)
; before and after the second call, and the POS/CTL data of custom
; channel 1 (+92).
; Correct: the moves behind the normal bank are not changed; custom
; channel 1 gets POS/CTL 0 (the blank sprite).
; @configs clib
DEFTYPE .l
Dim sd.l(63) : For i=0 To 63 : sd(i)=i*$01010101 : Next
s=Addr Sprite(0)
Poke.l s,&sd(0) : Poke.w s+4,10 : Poke.w s+6,1 : Poke.w s+8,0
Poke.w s+10,44 : Poke.w s+12,2 : Poke.w s+14,1
InitCopList 0,44,32,$10004,8,32,80
CustomSprites 0,0,14,8
c=Addr CopList(0)
p=Peek.l(c+12)+128 : q=Peek.l(c+92)+16
DisplaySprite 0,0,50,10,9
Dim o.l(7)
For i=0 To 7 : o(i)=Peek.l(p+i*4) : Next
DisplaySprite 0,0,50,100,9
k=0
For i=0 To 7
  If Peek.l(p+i*4)=o(i) Then k=k+1
Next
NPrint "moves behind the bank kept ",k," of 8"
NPrint "custom channel 1 pos and ctl 0 ",(Peek.w(q+2)=0) AND (Peek.w(q+6)=0)
End
