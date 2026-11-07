; blitzoslib-addbox-list-range (#989) - AddBox with a text counts the
; strings separated by | but stores the count as the box's range (the
; word at offset 14 of the box) only for the box types below 2 (CMP #2,
; btype : BPL).  A list box (box type 5) is drawn from the same text and
; uses range as its number of entries, but keeps 0.  SetBox on it always
; fails with "Value Out of Range" (the value is compared with range), and
; nothing is drawn.
; The type argument of AddBox is 2 * box type + the bevel bit, so 10 is a
; list box.  The page object is at Addr Page(0), its +4 is the address of
; the boxes.  Runtime error checking on (the error is raised by the
; debugger's requester).
; Correct: range 3, SetBox 0,1 selects entry 1.
; @pass runtime_errors=on
DEFTYPE .l
BitMap 0,320,200,2
InitPage 0,0,4
AddBox 0,0,10,10,100,40,10,"one|two|three"
b=Peek.l(Addr Page(0)+4)
NPrint "range ",Peek.w(b+14)
BLITZ
SetBox 0,1
v=BoxValue(0)
AMIGA
NPrint "value ",v
End
LoadShape 0,"x"
