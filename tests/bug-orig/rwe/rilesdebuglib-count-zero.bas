; rilesdebuglib-count-zero (#701) - D_HexWord, D_CheckNMove,
; D_CheckNMoveA and D_CheckNMoveWord count with DBRA from count-1: a
; count of 0 runs 65536 times (D_HexWord and the D_CheckNMove forms write
; 327680 bytes, D_CheckNMoveWord 65536); D_CheckNMoveWord doubles the
; count as a word, so $8001 words copy 2 bytes.  GetDataItem with size 0
; shifts 65536 bytes through the result, with size -1 65535 (its help
; text: "otherwise long").  The program gives the commands blocks large
; enough for the original's writes.
; Correct: a count of 0 writes nothing; $8001 words are 65538 bytes;
; sizes 0 and -1 read a long.
; @fixed #701
; @rwe rilesdebuglib
DEFTYPE .l
b=AllocMem_(400000,$10001)
s=AllocMem_(300000,$10001)
If b=0 OR s=0 Then NPrint "no memory" : End
Poke.b b,85 : D_HexWord s,b,0
If Peek.b(b)=85 Then NPrint "HexWord: nothing" Else NPrint "HexWord: written"
Poke.b b,85 : D_CheckNMove s,b,0
If Peek.b(b)=85 Then NPrint "CheckNMove: nothing" Else NPrint "CheckNMove: written"
Poke.b b,85 : D_CheckNMoveA s,b,0
If Peek.b(b)=85 Then NPrint "CheckNMoveA: nothing" Else NPrint "CheckNMoveA: written"
Poke.b b,85 : D_CheckNMoveWord s,b,0
If Peek.b(b)=85 Then NPrint "CheckNMoveWord: nothing" Else NPrint "CheckNMoveWord: written"
Poke.b b+65537,85 : D_CheckNMoveWord s,b,$8001
If Peek.b(b+65537)=85 Then NPrint "CheckNMoveWord $8001: short" Else NPrint "CheckNMoveWord $8001: all"
Poke.l s,$11223344
NPrint "size 0: ",Hex$(GetDataItem(s,0))
NPrint "size -1: ",Hex$(GetDataItem(s,-1))
FreeMem_ s,300000
FreeMem_ b,400000
End
