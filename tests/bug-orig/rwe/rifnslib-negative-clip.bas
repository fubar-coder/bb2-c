; rifnslib-negative-clip (#664) - FNSClip accepts a negative xmin; a
; character at x = -16 passes the x test, and its byte offset is computed
; from the long $0000FFF0 (ANDI.l #$fff0, ASR.w #3): $FFFE, 65534 bytes
; behind the start of its row.  Made safe: the bitmap is 640x1024 (80 KB
; per plane), so the character lands inside it, about 820 rows below the
; print row; the program looks for set pixels from row 40 on.
; Correct: nothing is drawn outside the rows of the text.
; rifnslib needs RIAmosFuncLib (FNSLoad and FNSUnload use its banks).
; @rwe rifnslib riamosfunclib
DEFTYPE .l
BitMap 0,640,1024,1
FNSOutput 0
FNSClip -32,0,639,1023
FNSInk 1
FNSPrint 0,-16,10,"x"
p=Peek.l(Addr BitMap(0)+8)
s=0
For i=40*80 To 1024*80-4 Step 4
  s=s OR Peek.l(p+i)
Next
If s Then NPrint "pixels far below the text" Else NPrint "nothing far below the text"
End
