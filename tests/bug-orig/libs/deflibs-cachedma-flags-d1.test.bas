; deflibs-cachedma-flags-d1 (#959) - exec's CachePreDMA and CachePostDMA
; take flags in d0 (exec_lib.fd: (address,length,flags)(a0,a1,d0)), but
; the deflibs declare d1: exec gets whatever is in d0.  As a statement or
; alone on the right side of an assignment the arguments are evaluated
; in d0 (the flags last), so d0 holds the flags by chance; inside an
; expression (y+...) they are evaluated in d1, d0 keeps y.
; Here, CachePreDMA and CachePostDMA of exec are replaced (SetFunction,
; under Forbid) by routines of the program that record d0; the program
; calls y+CachePreDMA_ with flags 2 (DMA_Continue) and y+CachePostDMA_
; with flags 8 (DMA_ReadFromRAM), y = 7, restores the vectors and checks
; the d0 seen.  The routines and their data are in the case, behind a
; jump.
; Correct: 2 and 8.  The original passes the flags in d1, exec sees 7.
DEFTYPE .l

!TestCase{flags_in_d0}
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
  a=Peek.l(?rec)
  b=Peek.l(?rec+4)
  !CheckEq{a,2,"pre_flags"}
  !CheckEq{b,8,"post_flags"}
  Goto cachedma_done

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
cachedma_done:
!EndTestCase
