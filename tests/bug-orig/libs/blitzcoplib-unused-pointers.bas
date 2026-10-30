; blitzcoplib-unused-pointers (#407) - Slice sets the pointers of the
; instructions its flags do not ask for to 0 (+22 BPLCON1 without flag
; bit 14), but the 'fixloop relocates all five pointers into the copper
; list (pointer - maxcop + position), the zero ones too; Show/ShowF/ShowB
; then write the BPLCON1 value through +22 to that random address.
; Here: the long form of Slice with flags $BFF0 (no BPLCON1), in Amiga
; mode; the program reads the pointers of the slice object (+10..+26)
; and does not call Show.
; Correct: the BPLCON1 pointer of the slice stays 0.
; @configs clib
DEFTYPE .l
Slice 0,44,320,200,$BFF0,2,8,4,320,320
o=Addr Slice(0)
NPrint "bitplanes ",Peek.l(o+10)<>0," sprites ",Peek.l(o+14)<>0," colours ",Peek.l(o+18)<>0," bplcon2 ",Peek.l(o+26)<>0
NPrint "bplcon1 pointer 0 ",Peek.l(o+22)=0
End
