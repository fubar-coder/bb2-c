; ilbmifflib-loadbitmap-nobody-fileopen (#1053) - LoadBitMap (ilbmbitmap)
; opens the file (ilbminit), reads the BMHD and searches the BODY; when
; there is none it goes to the runtime error "Can't Find IFF 'BODY'"
; without iffclose: the file stays open (iffiolib has no finit, also
; after the program ends).  The ILBM here (generated) has a BMHD and no
; BODY.  SetErr catches the error; then the program asks for an
; exclusive lock on the file, which fails while it is open (object in
; use).
; Correct: after the error the file is closed - the exclusive lock works.
; @fixed #1053
; @configs clib
; @pass file=nobody.iff|@TESTDATA@/ilbmifflib-loadbitmap-nobody-fileopen.ilbm
DEFTYPE .l
BitMap 0,16,1,1
SetErr
  NPrint "error"
  l=Lock_("CORPUS:nobody.iff",-1)
  NPrint "exclusive lock ",l<>0
  If l Then UnLock_ l
  End
End SetErr
LoadBitMap 0,"CORPUS:nobody.iff"
NPrint "loaded"
End
