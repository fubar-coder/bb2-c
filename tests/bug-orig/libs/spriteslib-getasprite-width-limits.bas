; spriteslib-getasprite-width-limits (#988) - GetaSprite splits a shape
; into hardware sprites of 16 pixels (SpriteMode 0): (width-1)/16+1, twice
; that for a 16 colour shape (attached pairs); ShowSprite needs
; channel + count <= 8.  The check of GetaSprite (gspchk, "Sprite too
; Wide") allows spritewid*32 pixels for a 4 colour shape (64 in mode 0,
; 4 sprites) and twice that for a 16 colour shape (128): the opposite of
; what the 8 channels allow (8 sprites of a 4 colour shape = 128 pixels,
; 4 pairs of a 16 colour shape = 64 pixels).  Here a 4 colour shape of
; 128 pixels (8 sprites), runtime error checking on (the debugger).
; The other direction, a 16 colour shape of 128 pixels, is in
; spriteslib-getasprite-width-16col.
; Correct: GetaSprite accepts it (8 channels do).  The original shows
; "Sprite too Wide" (a requester of the debugger).
; @pass runtime_errors=on
BitMap 0,128,16,2
Use BitMap 0
GetaShape 0,0,0,128,16
GetaSprite 0,0
NPrint "accepted"
End
