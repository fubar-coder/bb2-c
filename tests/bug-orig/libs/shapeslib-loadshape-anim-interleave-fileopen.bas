; shapeslib-loadshape-anim-interleave-fileopen (#1072) - LoadShape opens
; the file with ilbmifflib's ilbminit, decodes the first frame and then
; looks for an "ANHD" (an ANIM); when the ANHD's operation is 5 but its
; interleave (the byte at ANHD + 18) is not 1, it goes to the runtime
; error "Illegal ANIM interleave value" without iffclose: the file stays
; open (iffiolib has no finit, also after the program ends).  The ILBM
; here (generated) is 16 x 1 pixels with one plane and has an ANHD of
; operation 5 and interleave 0 behind its BODY.  SetErr catches the
; error; then the program asks for an exclusive lock on the file, which
; fails while it is open (object in use).  The sibling
; shapeslib-loadshape-anim-fileopen covers "ANIM format not supported".
; Correct: after the error the file is closed - the exclusive lock works.
; @configs clib
; @pass file=anim.iff|@TESTDATA@/shapeslib-loadshape-anim-interleave-fileopen.ilbm
DEFTYPE .l
SetErr
  NPrint "error"
  l=Lock_("CORPUS:anim.iff",-1)
  NPrint "exclusive lock ",l<>0
  If l Then UnLock_ l
  End
End SetErr
LoadShape 0,"CORPUS:anim.iff"
NPrint "loaded"
End
