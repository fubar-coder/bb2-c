; rilvcallhooklib-d7-upper-word (#737) - the listview hook computes the
; number of tree lines (indent / 16 - 1) as a long whose upper word is
; whatever the caller left in d7: with a negative d7 it draws no tree
; lines at all.  No listview needed: the program calls the hook
; (RetrieveCallHook) itself with an LV_DRAW message for a RastPort on a
; bitmap of its own (128 x 16, one plane), a field table with the indent
; word at node offset 16 and one text field (an empty text), a node with
; the indent 32 whose next node ends the list, and d7 = $FFFF0000.  It
; counts the set pixels in the indent (the first 32 columns).
; Correct: the two dotted tree lines and the branch are drawn.
; @fixed #737
; @rwe rilvcallhooklib
DEFTYPE .l
m=AllocMem_(1024,$10001)
pl=AllocMem_(256,$10003)
If m=0 OR pl=0 Then NPrint "no memory" : End
bm=m : rp=m+64 : ta=m+176 : hk=m+192 : msg=m+224 : tb=m+256 : nd=m+320
col=m+352 : dri=m+384 : tl=m+420 : es=m+440 : blk=m+512
InitBitMap_ bm,1,128,16
Poke.l bm+8,pl
InitRastPort_ rp
Poke.l rp+4,bm
n$="topaz.font"
For i=1 To Len(n$) : Poke.b m+600+i-1,Asc(Mid$(n$,i,1)) : Next
Poke.l ta,m+600 : Poke.w ta+4,8
f=OpenFont_(ta)
If f=0 Then NPrint "no font" : End
SetFont_ rp,f
Poke.w tb,16 : Poke.w tb+2,12
Poke.w tb+14,100 : Poke.w tb+16,8 : Poke.w tb+18,0 : Poke.w tb+20,0
Poke.w tb+28,0
Poke.l nd,tl : Poke.l nd+8,es : Poke.l nd+12,col : Poke.w nd+16,32
Poke.w col,1 : Poke.w col+2,0 : Poke.w col+4,1 : Poke.w col+6,0
Poke.l hk+16,tb
Poke.l msg,$202 : Poke.l msg+4,rp : Poke.l msg+8,dri
Poke.w msg+12,0 : Poke.w msg+14,0 : Poke.w msg+16,99 : Poke.w msg+18,9
Poke.l msg+20,0
Poke.l blk,hk : Poke.l blk+4,msg : Poke.l blk+8,nd : Poke.l blk+12,RetrieveCallHook
GetReg d0,blk
MOVEM.l d2-d7/a2-a6,-(a7)
MOVE.l d0,a3
MOVE.l (a3),a0
MOVE.l 4(a3),a1
MOVE.l 8(a3),a2
MOVE.l 12(a3),a3
MOVE.l #$FFFF0000,d7
JSR (a3)
MOVEM.l (a7)+,d2-d7/a2-a6
c=0
For r=0 To 15
  For i=0 To 3
    v=Peek.b(pl+r*16+i) AND 255
    For b=0 To 7
      If v AND 1 Then c=c+1
      v=v/2
    Next
  Next
Next
NPrint "tree pixels ",c
CloseFont_ f
FreeMem_ pl,256
FreeMem_ m,1024
End
