; shapeslib-loadshape-shortbody-fileopen (#1065) - LoadShape opens the
; file with ilbmifflib's ilbminit and reads the BODY byte by byte with
; ilbmgetbyte (getme, iffiolib iffbyte); when the file ends before the
; picture does, getme goes to the runtime error "Can't Read File" without
; iffclose: the file stays open (iffiolib has no finit, also after the
; program ends), as with LoadBitMap.  The ILBM here (generated) is 16 x 4
; pixels with one plane, and its BODY ends after 2 of the 8 bytes.  SetErr
; catches the error; then the program asks for an exclusive lock on the
; file, which fails while it is open (object in use).
; Correct: after the error the file is closed - the exclusive lock works.
!TestCase{shortbody_fileopen}
  SetErr
    l=Lock_("CORPUS:short.iff",-1)
    ok=(l<>0)
    If l Then UnLock_ l
    !Check{ok,"exclusive_lock"}
    End
  End SetErr
  LoadShape 0,"CORPUS:short.iff"
  !Check{0,"not_refused"}
!EndTestCase
