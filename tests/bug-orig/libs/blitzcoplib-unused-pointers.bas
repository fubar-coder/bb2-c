; blitzcoplib-unused-pointers (#407) - Slice sets the pointers of the
; instructions its flags do not ask for to 0 (+22 BPLCON1 without flag
; bit 14), but the 'fixloop relocates all five pointers into the copper
; list (pointer - maxcop + position), the zero ones too; Show/ShowF/ShowB
; then write the BPLCON1 value through +22 to that random address.
; Here: the long form of Slice with flags $BFF0 (no BPLCON1), in Amiga
; mode; the program reads the pointers of the slice object (+10..+26).
; Only where +22 points at the slice's own BPLCON1 copy (+30) does it
; call Show 0,5,0 and print that copy and the number of longs of the
; copper list other than the bitplane pointer moves that changed (the
; original's pointer is random: no Show there).
; Correct: the BPLCON1 pointer of the slice is not into the copper list
; but the slice's +30; Show writes the scroll value ($BB for x 5) there
; and changes nothing else in the list.
; @fixed #407
; @configs clib
DEFTYPE .l
BitMap 0,320,200,2
Slice 0,44,320,200,$BFF0,2,8,4,320,320
o=Addr Slice(0)
NPrint "bitplanes ",Peek.l(o+10)<>0," sprites ",Peek.l(o+14)<>0," colours ",Peek.l(o+18)<>0," bplcon2 ",Peek.l(o+26)<>0
p=Peek.l(o+22)
NPrint "bplcon1 pointer at the copy ",p=o+30
If p=o+30
  l=CopLoc : n=CopLen
  Dim c.l(n/4)
  For i=0 To n/4-1 : c(i)=Peek.l(l+i*4) : Next
  Show 0,5,0
  k=0
  For i=0 To n/4-1
    v=Peek.l(l+i*4) : r=(v LSR 16) AND $FFFF
    If v<>c(i) AND (r<$E0 OR r>$FE) Then k=k+1
  Next
  NPrint "show con1 ",Hex$(Peek.w(o+30) AND $FFFF)," other longs changed ",k
Else
  NPrint "no show"
EndIf
End
