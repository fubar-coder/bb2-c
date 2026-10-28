; t167_coplisterr - displaylib: a command on an uninitialised coplist is a
; runtime error (_checkcoplist, only with runtime error checking).
BitMap 0,32,8,2
InitCopList 0,3
DisplayBitMap 0,0
NPrint "ok"
DisplayBitMap 1,0
NPrint "not reached"
End
