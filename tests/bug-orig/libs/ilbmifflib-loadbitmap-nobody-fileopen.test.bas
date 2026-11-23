; ilbmifflib-loadbitmap-nobody-fileopen (#1053) - LoadBitMap (ilbmbitmap)
; opens the file (ilbminit), reads the BMHD and searches the BODY; when
; there is none it goes to the runtime error "Can't Find IFF 'BODY'"
; without iffclose: the file stays open (iffiolib has no finit, also
; after the program ends).  The ILBM here (generated) has a BMHD and no
; BODY.  SetErr catches the error; then the program asks for an
; exclusive lock on the file, which fails while it is open (object in
; use).
; Correct: after the error the file is closed - the exclusive lock works.
DEFTYPE .l

!TestCase{file_closed}
  BitMap 0,16,1,1
  SetErr
    l=Lock_("CORPUS:nobody.iff",-1)
    !Check{l<>0,"exclusive_lock"}
    If l Then UnLock_ l
    End
  End SetErr
  LoadBitMap 0,"CORPUS:nobody.iff"
  !Check{0,"loaded"}
!EndTestCase
