; rilvcallhooklib-lineptrn (#738) - the listview hook pokes the tree
; line pattern $AAAA into rp_LinePtrn and afterwards writes $FFFF, not
; the pattern the RastPort had; an item with an indent of 1-15 pixels
; (no tree lines) leaves the RastPort at $AAAA.  No listview needed: the
; program calls the hook (RetrieveCallHook) itself with an LV_DRAW
; message for a RastPort on a bitmap of its own (128 x 16, one plane)
; whose line pattern it set to $F0F0, a field table with the indent word
; at node offset 16 and one text field (an empty text), first for a node
; with the indent 8, then for one with the indent 32, and prints
; rp_LinePtrn after each call.
; Correct: the RastPort keeps its pattern $F0F0 in both cases.
; @fixed #738
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
Poke.w rp+34,$F0F0
n$="topaz.font"
For i=1 To Len(n$) : Poke.b m+600+i-1,Asc(Mid$(n$,i,1)) : Next
Poke.l ta,m+600 : Poke.w ta+4,8
f=OpenFont_(ta)
If f=0 Then NPrint "no font" : End
SetFont_ rp,f
Poke.w tb,16 : Poke.w tb+2,12
Poke.w tb+14,100 : Poke.w tb+16,8 : Poke.w tb+18,0 : Poke.w tb+20,0
Poke.w tb+28,0
Poke.l nd,tl : Poke.l nd+8,es : Poke.l nd+12,col
Poke.w col,1 : Poke.w col+2,0 : Poke.w col+4,1 : Poke.w col+6,0
Poke.l hk+16,tb
Poke.l msg,$202 : Poke.l msg+4,rp : Poke.l msg+8,dri
Poke.w msg+12,0 : Poke.w msg+14,0 : Poke.w msg+16,99 : Poke.w msg+18,9
Poke.l msg+20,0
Poke.l blk,hk : Poke.l blk+4,msg : Poke.l blk+8,nd : Poke.l blk+12,RetrieveCallHook
For k=1 To 2
  If k=1 Then Poke.w nd+16,8 Else Poke.w nd+16,32
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
  NPrint "indent ",Peek.w(nd+16),": pattern $",Hex$(Peek.w(rp+34) AND $FFFF)
Next
CloseFont_ f
FreeMem_ pl,256
FreeMem_ m,1024
End
