; displaylib-customcolors-flag-ignored (#951) - the form of CustomColors with
; thru256flag (_customcols2) sets the CopList's bot to 257 without
; reading the flag: thru256flag 0 acts as 1.  bot = 257 tells the next
; custom wait that the display is already past line 256, so a later
; custom command below line 256 gets no wait for line 255 ($FFE1FFFE)
; and its wait for line AND 255 is met in the upper part of the display.
; Amiga mode, the list is built in memory only (as in t164_display): an
; ECS CopList (ypos 44) with 20 custom cops; CustomColors at line 20
; with thru256flag 0, then at line 220 (264) at offset 4.
; Correct: with the flag 0 the second one starts with the wait for line
; 255, as after the form without the flag.
; @configs clib
DEFTYPE .l
InitPalette 0,32
InitCopList 0,44,256,3,8,8,20
c=Addr CopList(0)
cu=Peek.l(c+28)
CustomColors 0,0,20,0,0,1,0
CustomColors 0,4,220,0,0,1
NPrint "first ",Hex$(Peek.l(cu))
NPrint "second ",Hex$(Peek.l(cu+16))
End
