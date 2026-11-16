; ilbmifflib-loadbitmap-shortbody-fileopen - LoadBitMap (ilbmbitmap)
; reads the BODY byte by byte (getme, iffiolib iffbyte); when the file
; ends before the picture does it goes to the runtime error "Can't Read
; File" without iffclose: the file stays open (iffiolib has no finit,
; also after the program ends).  The ILBM here (generated) is 16 x 4
; pixels with one plane, and its BODY ends after 2 of the 8 bytes.  SetErr
; catches the error; then the program asks for an exclusive lock on the
; file, which fails while it is open (object in use).
; Correct: after the error the file is closed - the exclusive lock works.
; @configs clib
; @pass file=short.iff|@TESTDATA@/ilbmifflib-loadbitmap-shortbody-fileopen.ilbm
DEFTYPE .l
BitMap 0,16,4,1
SetErr
  NPrint "error"
  l=Lock_("CORPUS:short.iff",-1)
  NPrint "exclusive lock ",l<>0
  If l Then UnLock_ l
  End
End SetErr
LoadBitMap 0,"CORPUS:short.iff"
NPrint "loaded"
End
