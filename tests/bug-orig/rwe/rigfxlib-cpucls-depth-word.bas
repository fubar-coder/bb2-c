; rigfxlib-cpucls-depth-word (#679) - CPUCls takes the number of planes
; from the word at +4 of the bitmap (Flags << 8 | Depth) minus 1 as a
; DBRA counter: a depth of 0 clears 65536 "planes", the 8 plane pointers
; and the longs behind them; bitmap 0 has only plane 0, so the next
; "plane" is address 0 and the low memory (ExecBase at 4) is cleared -
; a crash (the session ends in a timeout).
; Correct: a bitmap of depth 0 has no plane to clear, the program goes on.
; rigfxlib needs RIAmosFuncLib (SaveCMAP calls its BSave).
; @rwe rigfxlib riamosfunclib
BitMap 0,320,16,1
o.l=Addr BitMap(0)
NPrint "before"
Poke.w o+4,0
CPUCls 0
Poke.w o+4,1
NPrint "after"
End
