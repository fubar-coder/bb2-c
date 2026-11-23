; rilvcallhooklib-right-bitmap (#740) - the listview hook places a right
; aligned BitMap by adding MaxX - width to its extent, which (unlike a
; text's) already starts at the field's MinX: the BitMap lands MinX
; pixels right of its place.  No listview needed: the program calls the
; hook (RetrieveCallHook) itself with an LV_DRAW message for a RastPort
; on a bitmap of its own (128 x 16, one plane), the item at x 20-79, a
; field table of one right aligned BitMap field and a node with a BitMap
; of 16 x 4 set pixels.  It counts the set pixels left of x 80 (the
; item) and from x 80 on.
; Correct: the BitMap is drawn at the item's right edge (64 pixels in
; the item, 0 behind it).
DEFTYPE .l

!TestCase{right_aligned}
  m=AllocMem_(1024,$10001)
  pl=AllocMem_(512,$10003)
  !Require{m<>0 AND pl<>0,"allocated"}
  bm=m : rp=m+64 : ta=m+176 : hk=m+192 : msg=m+224 : tb=m+256 : nd=m+320
  col=m+352 : dri=m+384 : sb=m+420 : blk=m+512
  sp=pl+256
  InitBitMap_ bm,1,128,16
  Poke.l bm+8,pl
  InitBitMap_ sb,1,16,4
  Poke.l sb+8,sp
  For i=0 To 7 : Poke.b sp+i,255 : Next
  InitRastPort_ rp
  Poke.l rp+4,bm
  n$="topaz.font"
  For i=1 To Len(n$) : Poke.b m+600+i-1,Asc(Mid$(n$,i,1)) : Next
  Poke.l ta,m+600 : Poke.w ta+4,8
  f=OpenFont_(ta)
  !Require{f<>0,"font_open"}
  SetFont_ rp,f
  Poke.w tb,0 : Poke.w tb+2,12
  Poke.w tb+14,100 : Poke.w tb+16,8 : Poke.w tb+18,1 : Poke.w tb+20,1
  Poke.w tb+28,0
  Poke.l nd+8,sb : Poke.l nd+12,col
  Poke.w col,1 : Poke.w col+2,0 : Poke.w col+4,1 : Poke.w col+6,0
  Poke.l hk+16,tb
  Poke.l msg,$202 : Poke.l msg+4,rp : Poke.l msg+8,dri
  Poke.w msg+12,20 : Poke.w msg+14,0 : Poke.w msg+16,79 : Poke.w msg+18,9
  Poke.l msg+20,0
  Poke.l blk,hk : Poke.l blk+4,msg : Poke.l blk+8,nd : Poke.l blk+12,RetrieveCallHook
  GetReg d0,blk
  MOVEM.l d2-d7/a2-a6,-(a7)
  MOVE.l d0,a3
  MOVE.l (a3),a0
  MOVE.l 4(a3),a1
  MOVE.l 8(a3),a2
  MOVE.l 12(a3),a3
  MOVEQ #0,d7
  JSR (a3)
  MOVEM.l (a7)+,d2-d7/a2-a6
  WaitBlit_
  c=0 : d=0
  For r=0 To 11
    For i=0 To 15
      v=Peek.b(pl+r*16+i) AND 255
      For b=0 To 7
        If v AND 1
          If i<10 Then c=c+1 Else d=d+1
        EndIf
        v=v/2
      Next
    Next
  Next
  CloseFont_ f
  FreeMem_ pl,512
  FreeMem_ m,1024
  !CheckEq{c,64,"in_the_item"}
  !CheckEq{d,0,"behind_the_item"}
!EndTestCase
