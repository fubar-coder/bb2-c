; stencillib-stencil-blitsize (#284) - Stencil ORs the planes of the
; bitmap into the stencil with one blit of BLTSIZE rows << 6 | words
; without a check: BLTSIZE has 10 bits for the rows and 6 for the words,
; so a bitmap of more than 1024 rows or more than 64 words (1024 pixels)
; gets a wrong blit size (from 65 words the words overflow into the
; rows) and the stencil is filled only in part.
; Here: bitmaps filled with colour 1 (one plane), Stencil of each, the
; first byte of the last row and the last byte of the stencil (+4 of the
; stencil object: its memory).  320x4 is the control, 1280x4 (80 words:
; the blit is 5 rows of 16 words), 16x1100 (the blit is 76 rows).
; Correct: every byte of the stencil is $FF.  The original leaves the
; last rows of the two big ones 0.
; @fixed #284
; @configs clib
DEFTYPE .l
Statement mkst{w.l,h.l}
  BitMap 0,w,h,1
  Use BitMap 0
  Boxf 0,0,w-1,h-1,1
  Stencil 0,0
  m=Peek.l(Addr Stencil(0)+4)
  b=w/8
  NPrint w,"x",h,": ",Hex$(Peek.b(m+b*(h-1)) AND 255)," ",Hex$(Peek.b(m+b*h-1) AND 255)
  Free BitMap 0
End Statement
mkst{320,4}
mkst{1280,4}
mkst{16,1100}
End
