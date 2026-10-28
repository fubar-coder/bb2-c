; collslib-scoll-btst (#110) - SColl(k1,k2) computes the CLXDAT bit of
; the sprite pair from k1/2 and k2/2 without sorting the channels: with
; channel 0/1 as the SECOND argument the wrong bit is tested - SColl(4,0)
; tests bit 11 (sprites 0-6) instead of 10 (sprites 0-4).  Here sprites
; on channels 0 and 4 overlap (Amiberry: collision_level=full); after a
; frame DoColl latches CLXDAT.  There is no sprite on channel 2, but
; SColl(2,0) tests bit 10 instead of 9 and reports the collision of 0
; and 4.
; Correct: SColl(4,0) = SColl(0,4) = -1 (true), SColl(0,2) = SColl(2,0)
; = 0.
; @configs clib
DEFTYPE .l
BitMap 0,320,200,2
BitMap 1,16,16,2
Use BitMap 1 : Boxf 0,0,15,15,3
GetaShape 0,0,0,16,16
GetaSprite 0,0
BLITZ
Slice 0,44,320,200,$fff8,2,8,8,320,320
Show 0
ShowSprite 0,50,50,0
ShowSprite 0,54,52,4
VWait 2
DoColl
VWait 2
DoColl
a=SColl(0,4) : b=SColl(4,0) : c=SColl(0,2) : d=SColl(2,0)
AMIGA
NPrint "0,4 ",a
NPrint "4,0 ",b
NPrint "0,2 ",c
NPrint "2,0 ",d
End
