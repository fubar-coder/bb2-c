; t158_clipblerr - 2dlib: ClipBlit of a shape that was never initialised
; (error check of ClipBlit: "SHAPE UNKNOWNST TO ME")
BitMap 0,64,32,2
Use BitMap 0
NPrint "before"
ClipBlit 5,10,10
NPrint "after"
End
