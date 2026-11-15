; iffmakelib-cmap-odd (#184) - makecolor writes a CMAP of 3 bytes per
; colour and no pad byte when that length is odd (an odd number of
; colours): the chunk behind it, the BODY of SaveBitmap/SaveShape/
; SaveScreen, starts at an odd offset - not a valid IFF file (iffparse
; and other readers fail). Blitz's own loaders still find it: ifffind
; searches byte by byte (#178).
; Here a bitmap of 16x1 pixels, 1 plane, row $A55A, saved with a palette
; of 3 colours; the program reads the file back: the offset of "BODY",
; the FORM length and the file size, then LoadBitMap of the file.
; Correct: a pad byte behind the CMAP, the BODY at an even offset (58),
; the FORM length is the file size - 8; the picture loads back.
; @fixed #184
; @configs clib
DEFTYPE .l
BitMap 0,16,1,1
Poke.w Peek.l(Addr BitMap(0)+8),$A55A
InitPalette 0,3
SaveBitmap 0,"RAM:bo184.iff",0
m=AllocMem(128,65536)
If ReadFile(0,"RAM:bo184.iff")
  n=Lof(0)
  ReadMem 0,m,n
  CloseFile 0
EndIf
o=-1
For i=12 To n-4
  If o<0 AND Peek.l(m+i)=$424F4459 Then o=i
Next
NPrint "body at ",o," form ",Peek.l(m+4)," file ",n
BitMap 1,16,1,1
LoadBitMap 1,"RAM:bo184.iff"
NPrint "row ",Right$(Hex$(Peek.w(Peek.l(Addr BitMap(1)+8)) AND $FFFF),4)
KillFile "RAM:bo184.iff"
FreeMem m,128
End
