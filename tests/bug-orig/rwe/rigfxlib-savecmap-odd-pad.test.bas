; rigfxlib-savecmap-odd-pad (#687) - SaveCMAP writes FORM ILBM, a BMHD
; chunk and a CMAP chunk of 3 bytes per colour, the file 3 * colours +
; 48 bytes long and the FORM length 8 less.  IFF pads a chunk of odd
; length with a byte its length does not count; with an odd number of
; colours (here 5: a CMAP of 15 bytes) SaveCMAP writes no pad byte, the
; file ends one byte short and the FORM length is odd.  The program reads
; the file back: its length, the FORM length (the long at 4) and the CMAP
; length (the long at 44).
; Correct: 64 bytes, FORM length 56, CMAP length 15 (the pad byte is not
; counted in the chunk).
DEFTYPE .l

!TestCase{odd_pad}
  InitPalette 0,5
  b=AllocMem_(256,$10001)
  !Require{b<>0,"memory"}
  r=SaveCMAP(0,"CORPUS:bug-orig-cmap-odd")
  !Require{r<>0,"written"}
  f=Open_("CORPUS:bug-orig-cmap-odd",1005)
  !Require{f<>0,"file"}
  n=Read_(f,b,256)
  Close_ f
  d=DeleteFile_("CORPUS:bug-orig-cmap-odd")
  fl=Peek.l(b+4)
  cl=Peek.l(b+44)
  FreeMem_ b,256
  !CheckEq{n,64,"file_length"}
  !CheckEq{fl,56,"form_length"}
  !CheckEq{cl,15,"cmap_length"}
!EndTestCase
