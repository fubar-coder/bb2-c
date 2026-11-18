; riintuilib-ipcolour-bgpen-a2 (#696) - IPColour fgpen,bgpen with a
; negative fgpen (unchanged) calls SetBPen with the caller's a2 as the
; RastPort: a2 is set to the Rast only together with SetAPen.  The
; Rast's BgPen (RastPort offset 26, 0 after IPBitMap's InitRastPort)
; stays 0, and SetBPen writes into the memory at a2.
; Correct: the Rast's BgPen is 2.
; @fixed #696
; @rwe riintuilib
BitMap 0,64,16,2
IPBitMap 0,0
Use Rast 0
IPColour -1,2
NPrint "bgpen ",Peek.b(Addr Rast(0)+26)
End
