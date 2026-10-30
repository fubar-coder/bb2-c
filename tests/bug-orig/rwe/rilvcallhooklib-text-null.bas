; rilvcallhooklib-text-null (#742) - for a text field the listview hook
; takes the long at the field's offset in the node as a string pointer
; without testing it for 0: a node without a text shows the bytes from
; address 0 on.  No listview needed: the program calls the hook
; (RetrieveCallHook) itself with an LV_DRAW message for a RastPort on a
; bitmap of its own (128 x 16, one plane), a field table of one text
; field at node offset 8 and a node whose text pointer is 0.  To make
; the read visible it puts "AB",0 at address 0 (unused on the Amiga; the
; old long is restored at once) and counts the set pixels afterwards.
; Correct: a text pointer 0 draws no text (0 pixels).
; @rwe rilvcallhooklib
DEFTYPE .l
m=AllocMem_(1024,$10001)
pl=AllocMem_(256,$10003)
If m=0 OR pl=0 Then NPrint "no memory" : End
bm=m : rp=m+64 : ta=m+176 : hk=m+192 : msg=m+224 : tb=m+256 : nd=m+320
col=m+352 : dri=m+384 : blk=m+512
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
Poke.w tb,0 : Poke.w tb+2,12
Poke.w tb+14,100 : Poke.w tb+16,8 : Poke.w tb+18,0 : Poke.w tb+20,0
Poke.w tb+28,0
Poke.l nd+8,0 : Poke.l nd+12,col
Poke.w col,1 : Poke.w col+2,0 : Poke.w col+4,1 : Poke.w col+6,0
Poke.l hk+16,tb
Poke.l msg,$202 : Poke.l msg+4,rp : Poke.l msg+8,dri
Poke.w msg+12,0 : Poke.w msg+14,0 : Poke.w msg+16,99 : Poke.w msg+18,9
Poke.l msg+20,0
Poke.l blk,hk : Poke.l blk+4,msg : Poke.l blk+8,nd : Poke.l blk+12,RetrieveCallHook
o=Peek.l(0)
Poke.l 0,$41420000
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
Poke.l 0,o
c=0
For i=0 To 255
  If Peek.b(pl+i)<>0 Then c=c+1
Next
NPrint "pixels ",c
CloseFont_ f
FreeMem_ pl,256
FreeMem_ m,1024
End
