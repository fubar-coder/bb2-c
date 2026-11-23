; 2dlib-circle-negative-y (#392) - the CPU plots of Circle (cplot) and
; Circlef (cplot2) reject y only with a signed compare against the clip
; height and then multiply the unsigned low word with the bytes per row:
; a point above the top edge (y = -1 = 65535) is written 65535 rows
; behind the plane (Circle) or the fill buffer (Circlef).
; Circle: plane 0 of a 16x100 bitmap (2 bytes per row) is pointed at a
; zeroed block of 140000 bytes, so that rows up to 65535 stay inside it;
; Circle 8,1,3 has points at y = -2 and -1.  Counted: bytes set inside the
; bitmap (rows 0-99) and behind it.
; Circlef: the clip width of the bitmap is set to 16384 for the command,
; so that its fill buffer (clip width / 8 * height = 204800 bytes, laid
; out with the 2 bytes per row of the bitmap) reaches beyond row 65535;
; a zeroed chip block of that size is freed just before, so the fill
; buffer is allocated there (at its start or 8 bytes in).  Counted: the
; bytes set from offset 1000 to 204799 of the block: behind the rows
; Circlef 8,1,3 uses (rows 0-5 of the buffer), before the end of the
; buffer (behind it lies the header of the free memory).
; Correct: points above the top edge are clipped, nothing is written
; behind the bitmap or the rows of the fill buffer.
DEFTYPE .l

!TestCase{circle_above_top}
  BitMap 0,16,100,1
  o=Addr BitMap(0)
  p0=Peek.l(o+8)
  m=AllocMem(140000,$10000)
  Poke.l o+8,m
  Circle 8,1,3,1
  Poke.l o+8,p0
  n0=0 : n1=0
  For i=0 To 199
    If Peek.b(m+i)<>0 Then n0=n0+1
  Next
  For i=200 To 139999
    If Peek.b(m+i)<>0 Then n1=n1+1
  Next
  FreeMem m,140000
  !Check{n0>0,"inside"}
  !CheckEq{n1,0,"behind"}
!EndTestCase

!TestCase{circlef_above_top}
  BitMap 0,16,100,1
  o=Addr BitMap(0)
  f=AllocMem(204808,$10002)
  FreeMem f,204808
  Poke.w o+56,16384
  Circlef 8,1,3,1
  Poke.w o+56,16
  n2=0
  For i=1000 To 204799
    If Peek.b(f+i)<>0 Then n2=n2+1
  Next
  !CheckEq{n2,0,"fill_behind"}
!EndTestCase
