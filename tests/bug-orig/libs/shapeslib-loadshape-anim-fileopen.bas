; shapeslib-loadshape-anim-fileopen - LoadShape opens the file with
; ilbmifflib's ilbminit, decodes the first frame and then looks for an
; "ANHD" (an ANIM); when the ANHD's operation is not 5 ("ANIM format not
; supported") or its interleave not 1 ("Illegal ANIM interleave value")
; it goes to the runtime error without iffclose: the file stays open
; (iffiolib has no finit, also after the program ends).  The ILBM here
; (generated) is 16 x 1 pixels with one plane and has an ANHD with
; operation 0 behind its BODY.  SetErr catches the error; then the
; program asks for an exclusive lock on the file, which fails while it is
; open (object in use).
; Correct: after the error the file is closed - the exclusive lock works.
; @configs clib
; @pass file=anim.iff|@TESTDATA@/shapeslib-loadshape-anim-fileopen.ilbm
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
