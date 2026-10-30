; displaylib-init-sprite-pointer (#413) - the init of displaylib writes
; the address of its blank sprite into the template of the sprite moves
; at +2/+6 (the data of the SPR0POS/SPR0CTL moves) instead of +10/+14
; (SPR0PTH/SPR0PTL); InitCopList copies the template for every channel.
; Amiga mode (the list is built in memory only, as in t164_display): a
; CopList with 8 sprite channels; the moves of channel 0 (+12 of the
; CopList: 16 bytes per channel, POS, CTL, PTH, PTL).
; Correct: the sprite pointer of an unused channel is the blank sprite
; (not 0), its position and control words are 0.
; @configs clib
DEFTYPE .l
InitCopList 0,44,32,$10004,8,32,80
p=Peek.l(Addr CopList(0)+12)
Print "regs ",Right$(Hex$(Peek.w(p)),4)," ",Right$(Hex$(Peek.w(p+4)),4)
NPrint " ",Right$(Hex$(Peek.w(p+8)),4)," ",Right$(Hex$(Peek.w(p+12)),4)
v=((Peek.w(p+10) AND $FFFF) LSL 16) | (Peek.w(p+14) AND $FFFF)
NPrint "pointer set ",v<>0
NPrint "pos and ctl 0 ",(Peek.w(p+2)=0) AND (Peek.w(p+6)=0)
End
