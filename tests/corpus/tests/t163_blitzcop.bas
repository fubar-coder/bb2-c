; t163_blitzcop - blitzcoplib in Amiga mode (the slices are built, never
; shown: Slice/ColSplit/CustomCop use their Amiga mode code, no custom
; register is written except by FreeSlices at the end, which puts the
; system list GfxBase->copinit back).  Quick and long Slice (lores,
; hires dual playfield, smooth, a slice below line 256 with the line 256
; wait, SetBPLCON0, SetBLWait), CopLoc/CopLen, ColSplit, CustomCop,
; Show/ShowF/ShowB in all forms.  Printed: the slice objects (pointers
; relative to CopLoc), the copper list (bitplane pointer data as "*"), the
; bitplane pointers as bitmap/plane/offset.
DEFTYPE .l
BitMap 0,320,200,2
BitMap 1,640,200,4
NPrint "start ",CopLen," ",Peek.l(CopLoc)," ",Peek.l(CopLoc+4)
Slice 0,44,2
sn=0 : Gosub slinfo : Gosub dumpcop
Show 0 : Gosub planes
Show 0,17,5 : Gosub planes
ColSplit 1,15,8,2,30 : Gosub dumpcop
CustomCop Mki$($180)+Mki$($123)+Mki$($182)+Mki$($456),100 : Gosub dumpcop
SetBPLCON0 $200
Slice 1,150,$1C
sn=1 : Gosub slinfo : Gosub dumpcop
; the quick slice has no playfield bits in its bitplanes word: ShowF/ShowB
; step over every second pointer (odd/even planes of a 2 plane bitmap)
ShowF 0 : Gosub planes
ShowF 0,33,2 : Gosub planes
ShowF 0,5,6,7 : Gosub planes
ShowB 0 : Gosub planes
ShowB 0,9,10 : Gosub planes
ShowB 0,11,12,13 : Gosub planes
SetBPLCON0 $300
SetBLWait $FFDFFFFE
Slice 2,262,336,30,$FFF8,2,1,4,352,384
sn=2 : Gosub slinfo : Gosub dumpcop
Show 0,3,1 : Gosub planes
ColSplit 0,1,2,3,10 : Gosub dumpcop
; the other slices' pointers moved with the inserts
sn=0 : Gosub slinfo : sn=1 : Gosub slinfo
FreeSlices
NPrint "freed ",CopLen," ",Peek.l(CopLoc)," ",Peek.l(CopLoc+4)," ",Peek.w(Addr Slice(0))," ",Peek.w(Addr Slice(1))
End

slinfo:
  s=Addr Slice(sn) : c=CopLoc
  Print "slice ",sn,": ",Peek.w(s)," ",Hex$(Peek.w(s+2) AND $FFFF)," ",Peek.w(s+4)," ",Peek.w(s+6)," ",Peek.w(s+8)
  Print " h ",Peek.w(s+32)," con1 ",Hex$(Peek.w(s+30) AND $FFFF)
  If Peek.w(s+4) AND 15 Then Print " bp ",Peek.l(s+10)-c
  If Peek.w(s+6) Then Print " spr ",Peek.l(s+14)-c
  If Peek.w(s+8) Then Print " col ",Peek.l(s+18)-c
  If Peek.w(s+2) AND $4000 Then Print " c1 ",Peek.l(s+22)-c
  If Peek.w(s+2) AND $2000 Then Print " c2 ",Peek.l(s+26)-c
  NPrint ""
Return

dumpcop:
  c=CopLoc : n=CopLen/4 : NPrint "cop ",n
  ; up to the end instruction: the longs behind it are allocated, never
  ; written (CopLen counts them, their contents depend on the memory)
  i=0 : e=0
  While i<n AND e=0
    v=Peek.l(c+i*4) : r=(v LSR 16) AND $FFFF
    If (r AND 1)=0 AND r>=$E0 AND r<=$FE Then h$=Right$(Hex$(r),4)+"****" Else h$=Hex$(v)
    Print h$," "
    If v=$FFFFFFFE Then e=1
    If (i AND 7)=7 Then NPrint ""
    i=i+1
  Wend
  NPrint "(+",n-i,")"
Return

; bitplane pointers of the used slice: bitmap.plane+offset
planes:
  s=Addr Slice(sn) : p=Peek.l(s+10) : Print "planes"
  For i=0 To 7
    r=Peek.w(p+i*8) AND $FFFF
    If r>=$E0 AND r<=$FE
      v=((Peek.w(p+i*8+2) AND $FFFF) LSL 16) | (Peek.w(p+i*8+6) AND $FFFF)
      f$="?"
      For bn=0 To 1
        bm=Addr BitMap(bn)
        For j=0 To Peek.b(bm+5)-1
          o=v-Peek.l(bm+8+j*4)
          If o>=0 AND o<(Peek.w(bm) AND $FFFF)*Peek.w(bm+2) Then f$=Str$(bn)+"."+Str$(j)+"+"+Str$(o)
        Next
      Next
      Print " ",Right$(Hex$(r),3),"=",f$
    EndIf
  Next
  NPrint " con1 ",Hex$(Peek.w(s+30) AND $FFFF)
Return
