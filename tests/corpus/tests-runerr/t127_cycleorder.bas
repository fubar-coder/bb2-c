; t127_cycleorder - palettelib: SetCycle not in order (runtime error from
; the normal code; the message is in the original's error check area)
SetCycle 0,0,1,4
NPrint "before"
SetCycle 0,3,1,4
NPrint "after"
End
