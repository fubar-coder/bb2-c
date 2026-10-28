; t125_copyerr - bitmaplib error check cbchk: CopyBitMap of a borrowed
; bitmap
Dim cm.l(100)
CludgeBitMap 0,32,8,1,&cm(0)
NPrint "before"
CopyBitMap 0,1
NPrint "after"
End
