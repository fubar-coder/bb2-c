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
; @fixed #687
; @rwe rigfxlib riamosfunclib
DEFTYPE .l
InitPalette 0,5
b=AllocMem_(256,$10001)
If b=0 Then NPrint "no memory" : End
r=SaveCMAP(0,"RAM:bug-orig-cmap-odd")
If r=0 Then NPrint "not written" : End
f=Open_("RAM:bug-orig-cmap-odd",1005)
If f=0 Then NPrint "no file" : End
n=Read_(f,b,256)
Close_ f
d=DeleteFile_("RAM:bug-orig-cmap-odd")
NPrint "file length ",n
NPrint "FORM length ",Peek.l(b+4)
NPrint "CMAP length ",Peek.l(b+44)
FreeMem_ b,256
End
