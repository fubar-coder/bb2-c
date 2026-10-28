; t147_spriteerr - spriteslib error check gspchk: GetaSprite of an 8
; colour shape
BitMap 0,32,8,3
Use BitMap 0
GetaShape 0,0,0,16,4
NPrint "before"
GetaSprite 0,0
NPrint "after"
End
