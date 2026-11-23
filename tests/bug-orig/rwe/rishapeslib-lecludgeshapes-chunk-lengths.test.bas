; rishapeslib-lecludgeshapes-chunk-lengths (#811) - LECludgeShapes walks
; an LE file in memory with assumed sizes: it skips the SHHD chunk as 20
; bytes whatever its length, and with a palette it passes the CMAP chunk
; by the colours read (3 bytes each), without its pad byte.  Two LE files
; (generated), each read into memory and cludged into shape 0: one with
; an SHHD of 24 bytes, one with a CMAP of 3 bytes and its pad byte
; (loaded with palette 0).
; Correct: both shapes are found (widths 16 and 24, the colour
; $F0,$80,$10); the original finds no SDAT and loads nothing.
; rishapeslib needs RIPackLib (PackPBM, UnPackPBM), which needs
; RIAmosFuncLib (banks).
DEFTYPE .l

!TestCase{chunk_lengths}
  InitPalette 0,2
  m=AllocMem_(1024,$10002)
  !Require{m<>0,"memory"}
  n$="CORPUS:a.le"
  f=Open_(&n$,1005)
  !Require{f<>0,"file_a"}
  r=Read_(f,m,1024)
  Close_ f
  LECludgeShapes 0,0,m,0
  w0=ShapeWidth(0)
  n=AllocMem_(1024,$10002)
  !Require{n<>0,"memory_b"}
  n$="CORPUS:b.le"
  f=Open_(&n$,1005)
  !Require{f<>0,"file_b"}
  r=Read_(f,n,1024)
  Close_ f
  LECludgeShapes 1,1,n,0
  w1=ShapeWidth(1)
  h$=Hex$(Peek.l(Peek.l(Addr Palette(0))+4))
  !CheckEq{w0,16,"shhd_24"}
  !CheckEq{w1,24,"cmap_3"}
  !CheckStrEq{h$,"F0F0F0F0","colour"}
!EndTestCase
