; deflibs-cachedma-flags-d1 (#959) - exec's CachePreDMA and CachePostDMA
; take flags in d0 (exec_lib.fd: (address,length,flags)(a0,a1,d0)), but
; the deflibs declare d1: exec gets whatever is in d0.  As a statement or
; alone on the right side of an assignment the arguments are evaluated
; in d0 (the flags last), so d0 holds the flags by chance; inside an
; expression (y+...) they are evaluated in d1, d0 keeps y.
; Here, CachePreDMA and CachePostDMA of exec are replaced (SetFunction,
; under Forbid) by routines of the program that record d0; the program
; calls y+CachePreDMA_ with flags 2 (DMA_Continue) and y+CachePostDMA_
; with flags 8 (DMA_ReadFromRAM), y = 7, restores the vectors and prints
; the d0 seen.
; Correct: 2 and 8.  The original passes the flags in d1, exec sees 7.
; @fixed #959
; @configs clib
; @pass timeout=60
DEFTYPE .l
l=16
y=7
Forbid_
e=Peek.l(4)
o=SetFunction_(e,-762,?pre)
p=SetFunction_(e,-768,?post)
x=y+CachePreDMA_(?buf,&l,2)
x=y+CachePostDMA_(?buf,&l,8)
x=SetFunction_(e,-762,o)
x=SetFunction_(e,-768,p)
Permit_
NPrint "CachePreDMA flags ",Peek.l(?rec)
NPrint "CachePostDMA flags ",Peek.l(?rec+4)
End

pre:
  LEA rec(pc),a1 : MOVE.l d0,(a1)
  MOVEQ #0,d0 : RTS
post:
  LEA rec+4(pc),a1 : MOVE.l d0,(a1)
  MOVEQ #0,d0 : RTS

rec:
  Dc.l $55555555,$55555555
buf:
  Dc.l 0,0,0,0
