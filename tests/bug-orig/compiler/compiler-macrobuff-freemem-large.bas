; compiler-macrobuff-freemem-large (#336) - the other direction of
; compiler-macrobuff-freemem: options with macrobuf=40000 (AmiBlitz3's
; default Macro Buffer) make blitz2 free 40000 bytes of the 8192 byte block
; at its end, memory behind the block that is not its own; exec's free
; list is corrupted (alert 8100 0005 at the exit or later).
; Correct: macrobuff is freed with the size it was allocated with.
; @fixed #336
; @pass wrap=@INJECT@|track
; @pass jobline=macrobuf=40000
NPrint "hello"
End
