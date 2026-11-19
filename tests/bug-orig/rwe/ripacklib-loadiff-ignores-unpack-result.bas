; ripacklib-loadiff-ignores-unpack-result (#771) - LoadIFF does not look
; at the result of UnpackIFF: a file that loads but is no ILBM leaves the
; bitmap unchanged, and LoadIFF returns -1.  (BankLimit 60 gives the
; original its bank 51, #770.)
; Correct: 0.
; @fixed #771
; @rwe ripacklib riamosfunclib
; @pass file=x.bin|@TESTDATA@/ripacklib-loadiff-ignores-unpack-result.bin
DEFTYPE .l
BankLimit 60
BitMap 0,320,16,1
r=LoadIFF("CORPUS:x.bin",0)
NPrint r
End
