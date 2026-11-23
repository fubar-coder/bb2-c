; ripacklib-loadiff-ignores-unpack-result (#771) - LoadIFF does not look
; at the result of UnpackIFF: a file that loads but is no ILBM leaves the
; bitmap unchanged, and LoadIFF returns -1.  (BankLimit 60 gives the
; original its bank 51, #770.)
; Correct: 0.
DEFTYPE .l

!TestCase{no_ilbm}
  BankLimit 60
  BitMap 0,320,16,1
  r=LoadIFF("CORPUS:x.bin",0)
  !CheckEq{r,0,"result"}
!EndTestCase
